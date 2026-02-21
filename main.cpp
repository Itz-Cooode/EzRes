/*
 * author: Itzcooode
 * 
 * supported games: valorant, cs2, fortnite, apex legends, 
 *                  rainbow six siege, overwatch 2, minecraft, roblox
 */
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include "config_patch.h"
#include "resource.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")

static const COLORREF CLR_BG = RGB(12, 12, 18);
static const COLORREF CLR_PANEL = RGB(20, 20, 30);
static const COLORREF CLR_CARD = RGB(26, 26, 40);
static const COLORREF CLR_BORDER = RGB(50, 50, 75);
static const COLORREF CLR_ACCENT = RGB(99, 102, 241);
static const COLORREF CLR_DANGER = RGB(239, 68, 68);
static const COLORREF CLR_SUCCESS = RGB(74, 222, 128);
static const COLORREF CLR_WARNING = RGB(251, 191, 36);
static const COLORREF CLR_TEXT = RGB(230, 230, 240);
static const COLORREF CLR_SUBTEXT = RGB(140, 140, 165);
static const COLORREF CLR_MUTED = RGB(60, 60, 88);

static const int WIN_W = 520;
static const int WIN_H = 830;
static const int GAREA_TOP = 458;
static const int GAREA_H = 328;
static const int GAREA_BOT = GAREA_TOP + GAREA_H;
static const int CARD_H = 76;
static const int FOOTER_Y = WIN_H - 20;

#define ID_CMB_MONITOR 201
#define ID_CMB_ASPECT 202
#define ID_CMB_RES 203
#define ID_CMB_HZ 204
#define ID_TIMER_MAIN 220
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_RESTORE 301
#define ID_TRAY_EXIT 302

struct AP {
  const wchar_t *lbl;
  double ratio;
};
static const AP ASPECTS[] = {{L"All", 0.0},         {L"4:3", 4.0 / 3},
                             {L"5:4", 5.0 / 4},     {L"16:9", 16.0 / 9},
                             {L"16:10", 16.0 / 10}, {L"21:9", 21.0 / 9},
                             {L"3:2", 3.0 / 2}};
static const int ASPECT_CNT = (int)(sizeof(ASPECTS) / sizeof(ASPECTS[0]));

struct DispMode {
  DWORD w, h, hz;
  wstring label() const {
    std::wostringstream s;
    s << w << L"x" << h << L"  @" << hz << L" Hz";
    return s.str();
  }
};

struct Monitor {
  wstring dev, name;
  DWORD nW = 0, nH = 0;
};

enum class CfgFmt { INI, ValveKV, ColonKV };

struct KeySpec {
  wstring key, val;
};

struct GameProfile {
  wstring name, tag;
  COLORREF clr;
  wstring cfgTpl, exeTpl;
  CfgFmt fmt;
  vector<KeySpec> keys;
  wstring cfgPath, exePath;
  vector<wstring> allCfgPaths;
  bool cfgFound = false, exeFound = false, patched = false;
  bool hovPatch = false, hovLaunch = false;
};

static HWND g_hWnd = nullptr;
static vector<Monitor> g_mons;
static vector<DispMode> g_all, g_filt;
static DEVMODE g_saved{};
static bool g_applied = false;
static wstring g_statTxt;
static COLORREF g_statClr = CLR_SUBTEXT;
static int g_scrollY = 0;
static vector<GameProfile> g_profs;
static HFONT g_fTitle, g_fBig, g_fMed, g_fSm, g_fTag;
static bool g_drag = false;
static POINT g_dragPt = {};
static NOTIFYICONDATA g_nid = {};
static bool g_trayAdded = false;

struct BtnSt {
  bool hov = false, press = false;
};
static BtnSt g_bApply, g_bRestore, g_bClose, g_bMin;

static void AddTrayIcon(HWND hWnd) {
  g_nid.cbSize = sizeof(NOTIFYICONDATA);
  g_nid.hWnd = hWnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
  wcscpy_s(g_nid.szTip, L"EzRes - Click to restore");
  Shell_NotifyIcon(NIM_ADD, &g_nid);
  g_trayAdded = true;
}

static void RemoveTrayIcon() {
  if (g_trayAdded) {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    g_trayAdded = false;
  }
}

static void MinimizeToTray(HWND hWnd) {
  ShowWindow(hWnd, SW_HIDE);
  if (!g_trayAdded)
    AddTrayIcon(hWnd);
}

static void RestoreFromTray(HWND hWnd) {
  ShowWindow(hWnd, SW_SHOW);
  SetForegroundWindow(hWnd);
  RemoveTrayIcon();
}

static HFONT MkFont(int pts, int w = FW_NORMAL, bool ital = false,
                    const wchar_t *face = L"Segoe UI") {
  return CreateFont(-MulDiv(pts, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72),
                    0, 0, 0, w, ital ? 1 : 0, 0, 0, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, face);
}
static wstring ExpandEnv(const wstring &s) {
  wchar_t b[MAX_PATH * 2] = {};
  ExpandEnvironmentStrings(s.c_str(), b, MAX_PATH * 2);
  return b;
}
static wstring ResolveWild(const wstring &tpl) {
  size_t star = tpl.find(L'*');
  if (star == wstring::npos)
    return tpl;
  size_t sep = tpl.rfind(L'\\', star);
  wstring dir = (sep != wstring::npos) ? tpl.substr(0, sep + 1) : L"";
  wstring rest = tpl.substr(star + 1);
  if (!rest.empty() && rest[0] == L'\\')
    rest = rest.substr(1);
  WIN32_FIND_DATA fd{};
  HANDLE h = FindFirstFile((dir + L"*").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE)
    return L"";
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      continue;
    wstring n = fd.cFileName;
    if (n == L"." || n == L"..")
      continue;
    wstring cand = dir + n + (rest.empty() ? L"" : L"\\" + rest);
    wstring res = ResolveWild(cand);
    if (!res.empty() &&
        GetFileAttributes(res.c_str()) != INVALID_FILE_ATTRIBUTES) {
      FindClose(h);
      return res;
    }
  } while (FindNextFile(h, &fd));
  FindClose(h);
  return L"";
}

static vector<wstring> ResolveWildAll(const wstring &tpl) {
  vector<wstring> results;
  size_t star = tpl.find(L'*');
  if (star == wstring::npos) {
    if (GetFileAttributes(tpl.c_str()) != INVALID_FILE_ATTRIBUTES)
      results.push_back(tpl);
    return results;
  }
  size_t sep = tpl.rfind(L'\\', star);
  wstring dir = (sep != wstring::npos) ? tpl.substr(0, sep + 1) : L"";
  wstring rest = tpl.substr(star + 1);
  if (!rest.empty() && rest[0] == L'\\')
    rest = rest.substr(1);
  WIN32_FIND_DATA fd{};
  HANDLE h = FindFirstFile((dir + L"*").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE)
    return results;
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      continue;
    wstring n = fd.cFileName;
    if (n == L"." || n == L"..")
      continue;
    wstring cand = dir + n + (rest.empty() ? L"" : L"\\" + rest);

    if (rest.find(L'*') != wstring::npos) {
      auto subRes = ResolveWildAll(cand);
      results.insert(results.end(), subRes.begin(), subRes.end());
    } else {
      if (GetFileAttributes(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
        results.push_back(cand);
      }
    }
  } while (FindNextFile(h, &fd));
  FindClose(h);
  return results;
}
static wstring ResolvePath(const wstring &t) {
  return ResolveWild(ExpandEnv(t));
}
static bool FileEx(const wstring &p) {
  return !p.empty() && GetFileAttributes(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static wstring GetSteamPath() {
  wchar_t b[MAX_PATH] = {};
  DWORD sz = sizeof(b);
  if (RegGetValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
                  L"InstallPath", RRF_RT_REG_SZ, nullptr, b,
                  &sz) == ERROR_SUCCESS &&
      b[0])
    return b;
  sz = sizeof(b);
  ZeroMemory(b, sizeof(b));
  RegGetValue(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath",
              RRF_RT_REG_SZ, nullptr, b, &sz);
  wstring p = b;
  for (auto &c : p)
    if (c == L'/')
      c = L'\\';
  return p;
}

static void InitProfiles() {
  wstring stm = GetSteamPath();

  g_profs.push_back({L"Valorant",
                     L"VAL",
                     RGB(255, 70, 84),
                     L"%LOCALAPPDATA%\\VALORANT\\Saved\\Config\\*"
                     L"\\WindowsClient\\GameUserSettings.ini",
                     L"%LOCALAPPDATA%"
                     L"\\VALORANT\\live\\ShooterGame\\Binaries\\Win64\\VALORANT"
                     L"-Win64-Shipping.exe",
                     CfgFmt::INI,
                     {
                         {L"ResolutionSizeX", L"W"},
                         {L"ResolutionSizeY", L"H"},
                         {L"LastUserConfirmedResolutionSizeX", L"W"},
                         {L"LastUserConfirmedResolutionSizeY", L"H"},
                         {L"DesiredScreenWidth", L"W"},
                         {L"DesiredScreenHeight", L"H"},
                         {L"LastUserConfirmedDesiredScreenWidth", L"W"},
                         {L"LastUserConfirmedDesiredScreenHeight", L"H"},
                         {L"bShouldLetterbox", L"False"},
                         {L"bLastConfirmedShouldLetterbox", L"False"},
                         {L"LastConfirmedFullscreenMode", L"2"},
                         {L"PreferredFullscreenMode", L"2"},
                         {L"FullscreenMode", L"2"},
                     }});

  g_profs.push_back({L"Counter-Strike 2",
                     L"CS2",
                     RGB(240, 165, 0),
                     stm + L"\\userdata\\*\\730\\local\\cfg\\video.txt",
                     stm + L"\\steamapps\\common\\Counter-Strike Global "
                           L"Offensive\\game\\bin\\win64\\cs2.exe",
                     CfgFmt::ValveKV,
                     {
                         {L"setting.defaultres", L"W"},
                         {L"setting.defaultresheight", L"H"},
                         {L"setting.fullscreen", L"1"},
                     }});

  g_profs.push_back(
      {L"Fortnite",
       L"FN",
       RGB(0, 180, 255),
       L"%LOCALAPPDATA%"
       L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini",
       L"%ProgramFiles%\\Epic "
       L"Games\\Fortnite\\FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-"
       L"Shipping.exe",
       CfgFmt::INI,
       {
           {L"ResolutionSizeX", L"W"},
           {L"ResolutionSizeY", L"H"},
           {L"LastUserConfirmedResolutionSizeX", L"W"},
           {L"LastUserConfirmedResolutionSizeY", L"H"},
           {L"FullscreenMode", L"1"},
           {L"LastConfirmedFullscreenMode", L"1"},
       }});

  g_profs.push_back(
      {L"Apex Legends",
       L"APX",
       RGB(205, 50, 30),
       L"%USERPROFILE%\\Saved Games\\Respawn\\Apex\\local\\videoconfig.txt",
       stm + L"\\steamapps\\common\\Apex Legends\\r5apex.exe",
       CfgFmt::ValveKV,
       {
           {L"setting.defaultres", L"W"},
           {L"setting.defaultresheight", L"H"},
           {L"setting.fullscreen", L"1"},
       }});

  g_profs.push_back(
      {L"Rainbow Six Siege",
       L"R6",
       RGB(30, 140, 220),
       L"%USERPROFILE%\\Documents\\My Games\\Rainbow Six - "
       L"Siege\\*\\GameSettings.ini",
       L"%ProgramFiles(x86)%\\Ubisoft\\Ubisoft Game Launcher\\games\\Tom "
       L"Clancy's Rainbow Six Siege\\RainbowSix.exe",
       CfgFmt::INI,
       {
           {L"Width", L"W"},
           {L"Height", L"H"},
           {L"WindowStyle", L"2"},
       }});

  g_profs.push_back({L"Overwatch 2",
                     L"OW2",
                     RGB(250, 165, 0),
                     L"%LOCALAPPDATA%\\Blizzard "
                     L"Entertainment\\Overwatch\\Settings\\Settings_v0.ini",
                     L"%ProgramFiles(x86)%\\Overwatch\\_retail_\\Overwatch.exe",
                     CfgFmt::INI,
                     {
                         {L"WindowWidth", L"W"},
                         {L"WindowHeight", L"H"},
                         {L"DisplayMode", L"2"},
                     }});

  g_profs.push_back(
      {L"Minecraft Java",
       L"MC",
       RGB(100, 200, 80),
       L"%APPDATA%\\.minecraft\\options.txt",
       L"%ProgramFiles%\\Minecraft Launcher\\MinecraftLauncher.exe",
       CfgFmt::ColonKV,
       {
           {L"overrideWidth", L"W"},
           {L"overrideHeight", L"H"},
       }});

  g_profs.push_back(
      {L"Roblox",
       L"ROB",
       RGB(226, 35, 26),
       L"",
       L"%LOCALAPPDATA%\\Roblox\\Versions\\*\\RobloxPlayerBeta.exe",
       CfgFmt::INI,
       {}});
}

static void ResolveProfiles() {
  for (auto &p : g_profs) {
    if (!p.cfgTpl.empty()) {
      wstring expandedTpl = ExpandEnv(p.cfgTpl);
      p.allCfgPaths = ResolveWildAll(expandedTpl);
      if (!p.allCfgPaths.empty()) {
        p.cfgPath = p.allCfgPaths[0];
        p.cfgFound = true;
      }
    }
    if (!p.exeTpl.empty()) {
      p.exePath = ResolvePath(p.exeTpl);
      p.exeFound = FileEx(p.exePath);
    }
  }
}

static bool DoPatch(GameProfile &p, DWORD w, DWORD h) {
  if (!p.cfgFound)
    return false;
  map<wstring, wstring> vals;
  for (auto &k : p.keys) {
    if (k.val == L"W")
      vals[k.key] = std::to_wstring(w);
    else if (k.val == L"H")
      vals[k.key] = std::to_wstring(h);
    else
      vals[k.key] = k.val;
  }

  int successCount = 0;

  for (auto &cfgPath : p.allCfgPaths) {
    bool ok = false;
    switch (p.fmt) {
    case CfgFmt::INI:
      ok = PatchIni(cfgPath, vals);
      break;
    case CfgFmt::ValveKV:
      ok = PatchValveKV(cfgPath, vals);
      break;
    case CfgFmt::ColonKV:
      ok = PatchColonKV(cfgPath, vals);
      break;
    }
    if (ok) {
      successCount++;
    }
  }

  if (successCount > 0)
    p.patched = true;
  return successCount > 0;
}

static void EnumMons() {
  g_mons.clear();
  DISPLAY_DEVICE dd{};
  dd.cb = sizeof(dd);
  for (int i = 0; EnumDisplayDevices(nullptr, i, &dd, 0); ++i) {
    if (!(dd.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
      ZeroMemory(&dd, sizeof(dd));
      dd.cb = sizeof(dd);
      continue;
    }
    Monitor m;
    m.dev = dd.DeviceName;
    m.name = dd.DeviceString;
    DEVMODE dm{};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
      m.nW = dm.dmPelsWidth;
      m.nH = dm.dmPelsHeight;
    }
    g_mons.push_back(m);
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
  }
  if (g_mons.empty()) {
    Monitor m;
    m.dev = L"\\\\.\\DISPLAY1";
    m.name = L"Primary Display";
    m.nW = GetSystemMetrics(SM_CXSCREEN);
    m.nH = GetSystemMetrics(SM_CYSCREEN);
    g_mons.push_back(m);
  }
}
static void EnumModes(const wstring &dev) {
  g_all.clear();
  DEVMODE dm{};
  dm.dmSize = sizeof(dm);
  for (DWORD i = 0; EnumDisplaySettings(dev.c_str(), i, &dm); ++i) {
    if (dm.dmBitsPerPel < 16)
      continue;
    DispMode m{dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency};
    bool dup = false;
    for (auto &e : g_all)
      if (e.w == m.w && e.h == m.h && e.hz == m.hz) {
        dup = true;
        break;
      }
    if (!dup)
      g_all.push_back(m);
  }
  std::sort(g_all.begin(), g_all.end(), [](auto &a, auto &b) {
    if (a.w != b.w)
      return a.w > b.w;
    if (a.h != b.h)
      return a.h > b.h;
    return a.hz > b.hz;
  });
}
static void FilterModes(double ratio) {
  g_filt.clear();
  for (auto &m : g_all) {
    if (ratio > 0.0 && std::fabs((double)m.w / m.h - ratio) > 0.05)
      continue;
    bool dup = false;
    for (auto &b : g_filt)
      if (b.w == m.w && b.h == m.h) {
        dup = true;
        break;
      }
    if (!dup)
      g_filt.push_back(m);
  }
}
static int MonSel(HWND h) {
  return (int)SendDlgItemMessage(h, ID_CMB_MONITOR, CB_GETCURSEL, 0, 0);
}
static int AspSel(HWND h) {
  return (int)SendDlgItemMessage(h, ID_CMB_ASPECT, CB_GETCURSEL, 0, 0);
}
static int ResSel(HWND h) {
  return (int)SendDlgItemMessage(h, ID_CMB_RES, CB_GETCURSEL, 0, 0);
}
static void PopHz(HWND hWnd) {
  HWND hHz = GetDlgItem(hWnd, ID_CMB_HZ);
  SendMessage(hHz, CB_RESETCONTENT, 0, 0);
  int ri = ResSel(hWnd);
  if (ri < 0 || (size_t)ri >= g_filt.size())
    return;
  auto &sel = g_filt[ri];
  vector<DWORD> hzs;
  for (auto &m : g_all)
    if (m.w == sel.w && m.h == sel.h) {
      bool dup = false;
      for (auto x : hzs)
        if (x == m.hz) {
          dup = true;
          break;
        }
      if (!dup)
        hzs.push_back(m.hz);
    }
  std::sort(hzs.rbegin(), hzs.rend());
  for (auto hz : hzs) {
    wstring s = std::to_wstring(hz) + L" Hz";
    SendMessage(hHz, CB_ADDSTRING, 0, (LPARAM)s.c_str());
  }
  if (!hzs.empty())
    SendMessage(hHz, CB_SETCURSEL, 0, 0);
}
static void PopRes(HWND hWnd) {
  HWND hR = GetDlgItem(hWnd, ID_CMB_RES);
  SendMessage(hR, CB_RESETCONTENT, 0, 0);
  for (auto &m : g_filt)
    SendMessage(hR, CB_ADDSTRING, 0, (LPARAM)m.label().c_str());
  if (!g_filt.empty())
    SendMessage(hR, CB_SETCURSEL, 0, 0);
  PopHz(hWnd);
}

static void SetStat(const wstring &t, COLORREF c) {
  g_statTxt = t;
  g_statClr = c;
  InvalidateRect(g_hWnd, nullptr, FALSE);
}
static void GetSelRes(HWND hWnd, DWORD &outW, DWORD &outH, DWORD &outHz) {
  outW = outH = outHz = 0;
  int ri = ResSel(hWnd);
  if (ri < 0 || (size_t)ri >= g_filt.size())
    return;
  outW = g_filt[ri].w;
  outH = g_filt[ri].h;
  HWND hHz = GetDlgItem(hWnd, ID_CMB_HZ);
  int hi = (int)SendMessage(hHz, CB_GETCURSEL, 0, 0);
  wchar_t hb[32] = {};
  SendMessage(hHz, CB_GETLBTEXT, hi, (LPARAM)hb);
  outHz = (DWORD)_wtoi(hb);
  if (!outHz)
    outHz = g_filt[ri].hz;
}
static void ApplyRes(HWND hWnd) {
  int mi = MonSel(hWnd);
  if (mi < 0 || (size_t)mi >= g_mons.size()) {
    SetStat(L"[!] No monitor selected.", CLR_DANGER);
    return;
  }
  DWORD w, h, hz;
  GetSelRes(hWnd, w, h, hz);
  if (!w) {
    SetStat(L"[!] No resolution selected.", CLR_DANGER);
    return;
  }
  if (!g_applied) {
    g_saved.dmSize = sizeof(g_saved);
    EnumDisplaySettings(g_mons[mi].dev.c_str(), ENUM_CURRENT_SETTINGS,
                        &g_saved);
  }
  DEVMODE dm{};
  dm.dmSize = sizeof(dm);
  dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
  dm.dmPelsWidth = w;
  dm.dmPelsHeight = h;
  dm.dmDisplayFrequency = hz;
  LONG r = ChangeDisplaySettingsEx(g_mons[mi].dev.c_str(), &dm, nullptr,
                                   CDS_FULLSCREEN, nullptr);
  if (r == DISP_CHANGE_SUCCESSFUL) {
    g_applied = true;
    std::wostringstream ss;
    ss << L"[OK] Applied " << w << L"x" << h << L" @" << hz
       << L"Hz - now patch your game below";
    SetStat(ss.str(), CLR_SUCCESS);
  } else {
    std::wostringstream ss;
    ss << L"[X] Failed (code " << r << L"). Try Run as Administrator.";
    SetStat(ss.str(), CLR_DANGER);
  }
}
static void RestoreRes(HWND hWnd) {
  int mi = MonSel(hWnd);
  if (mi < 0 || (size_t)mi >= g_mons.size())
    return;
  if (g_applied)
    ChangeDisplaySettingsEx(g_mons[mi].dev.c_str(), &g_saved, nullptr,
                            CDS_FULLSCREEN, nullptr);
  else
    ChangeDisplaySettingsEx(g_mons[mi].dev.c_str(), nullptr, nullptr, 0,
                            nullptr);
  g_applied = false;
  SetStat(L"[OK] Restored to native resolution.", CLR_SUCCESS);
}

static void DrawRR(HDC hdc, RECT r, int rx, int ry, COLORREF fill,
                   COLORREF brd = 0, int bw = 0) {
  HBRUSH hb = CreateSolidBrush(fill);
  HPEN hp =
      (bw > 0) ? CreatePen(PS_SOLID, bw, brd) : (HPEN)GetStockObject(NULL_PEN);
  SelectObject(hdc, hb);
  SelectObject(hdc, hp);
  RoundRect(hdc, r.left, r.top, r.right, r.bottom, rx, ry);
  DeleteObject(hb);
  if (bw > 0)
    DeleteObject(hp);
}
static void DrawBtn(HDC hdc, RECT r, const wstring &txt, COLORREF bg,
                    COLORREF fg, HFONT font, bool hov, bool prs, int rad = 10) {
  COLORREF c = bg;
  if (prs)
    c = RGB(max(0, GetRValue(bg) - 30), max(0, GetGValue(bg) - 30),
            max(0, GetBValue(bg) - 30));
  else if (hov)
    c = RGB(min(255, GetRValue(bg) + 25), min(255, GetGValue(bg) + 25),
            min(255, GetBValue(bg) + 25));
  DrawRR(hdc, r, rad, rad, c, CLR_BORDER, 1);
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, fg);
  SelectObject(hdc, font);
  DrawText(hdc, txt.c_str(), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
static void ClipToRect(HDC hdc, const RECT &r) {
  HRGN rgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
  SelectClipRgn(hdc, rgn);
  DeleteObject(rgn);
}

static int CardY(int i) { return GAREA_TOP + i * CARD_H - g_scrollY; }
static int MaxScroll() {
  int total = (int)g_profs.size() * CARD_H;
  int vis = GAREA_H;
  return max(0, total - vis);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

  case WM_CREATE: {
    HWND hM = CreateWindow(
        WC_COMBOBOX, nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 118,
        WIN_W - 40, 240, hWnd, (HMENU)ID_CMB_MONITOR, nullptr, nullptr);
    SetWindowTheme(hM, L"DarkMode_CFD", nullptr);
    SendMessage(hM, WM_SETFONT, (WPARAM)g_fMed, TRUE);

    HWND hA = CreateWindow(
        WC_COMBOBOX, nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 208, 220,
        240, hWnd, (HMENU)ID_CMB_ASPECT, nullptr, nullptr);
    SetWindowTheme(hA, L"DarkMode_CFD", nullptr);
    SendMessage(hA, WM_SETFONT, (WPARAM)g_fMed, TRUE);
    for (int i = 0; i < ASPECT_CNT; ++i)
      SendMessage(hA, CB_ADDSTRING, 0, (LPARAM)ASPECTS[i].lbl);
    SendMessage(hA, CB_SETCURSEL, 0, 0);

    HWND hR = CreateWindow(
        WC_COMBOBOX, nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 298,
        WIN_W - 40, 400, hWnd, (HMENU)ID_CMB_RES, nullptr, nullptr);
    SetWindowTheme(hR, L"DarkMode_CFD", nullptr);
    SendMessage(hR, WM_SETFONT, (WPARAM)g_fMed, TRUE);

    HWND hHz = CreateWindow(
        WC_COMBOBOX, nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 248, 208,
        WIN_W - 268, 240, hWnd, (HMENU)ID_CMB_HZ, nullptr, nullptr);
    SetWindowTheme(hHz, L"DarkMode_CFD", nullptr);
    SendMessage(hHz, WM_SETFONT, (WPARAM)g_fMed, TRUE);

    EnumMons();
    for (auto &m : g_mons) {
      wstring lbl = m.name;
      if (m.nW)
        lbl += L"  (" + std::to_wstring(m.nW) + L"x" + std::to_wstring(m.nH) +
               L")";
      SendMessage(hM, CB_ADDSTRING, 0, (LPARAM)lbl.c_str());
    }
    SendMessage(hM, CB_SETCURSEL, 0, 0);
    if (!g_mons.empty()) {
      EnumModes(g_mons[0].dev);
      FilterModes(0.0);
      PopRes(hWnd);
    }

    InitProfiles();
    ResolveProfiles();

    g_statTxt = L"Select resolution, click Apply, then patch your game.";
    g_statClr = CLR_SUBTEXT;
    return 0;
  }

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT wrc;
    GetClientRect(hWnd, &wrc);
    HDC mdc = CreateCompatibleDC(hdc);
    HBITMAP mbmp = CreateCompatibleBitmap(hdc, wrc.right, wrc.bottom);
    HBITMAP obmp = (HBITMAP)SelectObject(mdc, mbmp);

    HBRUSH bgBr = CreateSolidBrush(CLR_BG);
    FillRect(mdc, &wrc, bgBr);
    DeleteObject(bgBr);

    RECT tbar = {0, 0, wrc.right, 72};
    HBRUSH pb = CreateSolidBrush(CLR_PANEL);
    FillRect(mdc, &tbar, pb);
    DeleteObject(pb);
    RECT stripe = {0, 68, wrc.right, 72};
    pb = CreateSolidBrush(CLR_ACCENT);
    FillRect(mdc, &stripe, pb);
    DeleteObject(pb);
    SetBkMode(mdc, TRANSPARENT);
    SelectObject(mdc, g_fTitle);
    SetTextColor(mdc, CLR_TEXT);
    RECT tr = {18, 8, 400, 48};
    DrawText(mdc, L"EzRes", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(mdc, g_fSm);
    SetTextColor(mdc, CLR_SUBTEXT);
    RECT sr = {20, 44, 420, 66};
    DrawText(mdc, L"Patch game configs + change display resolution", -1, &sr,
             DT_LEFT | DT_TOP | DT_SINGLELINE);

    {
      RECT rc2 = {wrc.right - 34, 14, wrc.right - 6, 38};
      RECT rm = {wrc.right - 70, 14, wrc.right - 38, 38};
      DrawBtn(mdc, rm, L"-", CLR_CARD, CLR_SUBTEXT, g_fSm, g_bMin.hov,
              g_bMin.press, 7);
      DrawBtn(mdc, rc2, L"X", CLR_DANGER, CLR_TEXT, g_fSm, g_bClose.hov,
              g_bClose.press, 7);
    }

    DrawRR(mdc, {12, 78, wrc.right - 12, 155}, 10, 10, CLR_CARD, CLR_BORDER, 1);
    SelectObject(mdc, g_fBig);
    SetTextColor(mdc, CLR_ACCENT);
    RECT lm = {22, 83, 300, 103};
    DrawText(mdc, L"MONITOR", -1, &lm, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawRR(mdc, {12, 162, wrc.right - 12, 248}, 10, 10, CLR_CARD, CLR_BORDER,
           1);
    SelectObject(mdc, g_fBig);
    SetTextColor(mdc, CLR_ACCENT);
    RECT la = {22, 167, 240, 187};
    DrawText(mdc, L"ASPECT RATIO", -1, &la,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT lh = {250, 167, wrc.right - 12, 187};
    DrawText(mdc, L"REFRESH RATE", -1, &lh,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawRR(mdc, {12, 255, wrc.right - 12, 340}, 10, 10, CLR_CARD, CLR_BORDER,
           1);
    SelectObject(mdc, g_fBig);
    SetTextColor(mdc, CLR_ACCENT);
    RECT lr = {22, 260, 300, 280};
    DrawText(mdc, L"RESOLUTION", -1, &lr,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawBtn(mdc, {20, 348, 250, 394}, L"Apply Stretch", CLR_ACCENT,
            CLR_TEXT, g_fBig, g_bApply.hov, g_bApply.press, 12);
    COLORREF resBg = g_applied ? CLR_DANGER : CLR_MUTED;
    COLORREF resFg = g_applied ? CLR_TEXT : CLR_SUBTEXT;
    DrawBtn(mdc, {258, 348, wrc.right - 20, 394}, L"Restore Native", resBg,
            resFg, g_fBig, g_bRestore.hov, g_bRestore.press, 12);

    if (g_applied) {
      HBRUSH db = CreateSolidBrush(CLR_SUCCESS);
      SelectObject(mdc, db);
      Ellipse(mdc, 22, 404, 34, 416);
      DeleteObject(db);
    }
    SelectObject(mdc, g_fMed);
    SetTextColor(mdc, g_statClr);
    RECT str2 = {g_applied ? 40 : 20, 399, wrc.right - 20, 422};
    DrawText(mdc, g_statTxt.c_str(), -1, &str2,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawRR(mdc, {12, 427, wrc.right - 12, 453}, 8, 8, RGB(40, 35, 8),
           CLR_WARNING, 1);
    SelectObject(mdc, g_fSm);
    SetTextColor(mdc, CLR_WARNING);
    RECT gt = {22, 430, wrc.right - 20, 450};
    DrawText(mdc,
             L"TIP: Set GPU Control Panel > Scaling Mode = Full Panel / "
             L"Stretched (NVIDIA/AMD)",
             -1, &gt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    {
      HPEN lp = CreatePen(PS_SOLID, 1, CLR_BORDER);
      SelectObject(mdc, lp);
      MoveToEx(mdc, 12, 458, nullptr);
      LineTo(mdc, wrc.right - 12, 458);
      DeleteObject(lp);
    }
    SelectObject(mdc, g_fBig);
    SetTextColor(mdc, CLR_TEXT);
    RECT gh = {20, 460, wrc.right - 20, 480};
    DrawText(mdc, L"GAME PROFILES - click Patch Config to apply stretch", -1,
             &gh, DT_LEFT | DT_TOP | DT_SINGLELINE);

    HRGN clip = CreateRectRgn(0, GAREA_TOP, wrc.right, GAREA_BOT);
    SelectClipRgn(mdc, clip);
    DeleteObject(clip);

    int scrollbarX = wrc.right - 8;
    for (int i = 0; i < (int)g_profs.size(); ++i) {
      auto &p = g_profs[i];
      int y = CardY(i);
      if (y + CARD_H < GAREA_TOP || y > GAREA_BOT)
        continue;

      RECT card = {12, y, wrc.right - 16, y + CARD_H - 3};
      DrawRR(mdc, card, 10, 10, CLR_CARD, CLR_BORDER, 1);

      RECT badge = {22, y + 14, 62, y + CARD_H - 14};
      DrawRR(mdc, badge, 8, 8, p.clr, 0, 0);
      SelectObject(mdc, g_fTag);
      SetBkMode(mdc, TRANSPARENT);
      SetTextColor(mdc, CLR_TEXT);
      DrawText(mdc, p.tag.c_str(), -1, &badge,
               DT_CENTER | DT_VCENTER | DT_SINGLELINE);

      SelectObject(mdc, g_fBig);
      SetTextColor(mdc, CLR_TEXT);
      RECT nm = {72, y + 10, wrc.right - 220, y + 30};
      DrawText(mdc, p.name.c_str(), -1, &nm,
               DT_LEFT | DT_VCENTER | DT_SINGLELINE);

      SelectObject(mdc, g_fSm);
      if (p.patched) {
        SetTextColor(mdc, CLR_SUCCESS);
        RECT st = {72, y + 32, wrc.right - 220, y + 50};
        wstring txt = L"[OK] Patched " + std::to_wstring(p.allCfgPaths.size()) + L" profile(s) - restart game";
        DrawText(mdc, txt.c_str(), -1, &st,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      } else if (p.cfgFound) {
        SetTextColor(mdc, CLR_SUCCESS);
        RECT st = {72, y + 32, wrc.right - 220, y + 50};
        wstring txt = L"[OK] Found " + std::to_wstring(p.allCfgPaths.size()) + L" config file(s)";
        DrawText(mdc, txt.c_str(), -1, &st,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      } else if (p.cfgTpl.empty()) {
        SetTextColor(mdc, CLR_SUBTEXT);
        RECT st = {72, y + 32, wrc.right - 220, y + 50};
        DrawText(mdc, L"[!] Direct config patch not supported", -1, &st,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      } else {
        SetTextColor(mdc, CLR_SUBTEXT);
        RECT st = {72, y + 32, wrc.right - 220, y + 50};
        DrawText(mdc, L"[X] Not installed / config not found", -1, &st,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      }

      bool canPatch = p.cfgFound && !p.cfgTpl.empty();
      COLORREF pbg = canPatch ? CLR_ACCENT : CLR_MUTED;
      COLORREF pfg = canPatch ? CLR_TEXT : CLR_SUBTEXT;
      RECT pb2 = {wrc.right - 206, y + 18, wrc.right - 110, y + CARD_H - 18};
      DrawBtn(mdc, pb2, L"Patch Config", pbg, pfg, g_fSm,
              p.hovPatch && canPatch, false, 8);

      bool canLaunch = p.exeFound;
      COLORREF lbg = canLaunch ? RGB(45, 45, 65) : CLR_MUTED;
      COLORREF lfg = canLaunch ? CLR_TEXT : CLR_SUBTEXT;
      RECT lb = {wrc.right - 104, y + 18, wrc.right - 20, y + CARD_H - 18};
      DrawBtn(mdc, lb, L"Launch", lbg, lfg, g_fSm, p.hovLaunch && canLaunch,
              false, 8);
    }

    SelectClipRgn(mdc, nullptr);
    int mx = MaxScroll();
    if (mx > 0) {
      int barH = GAREA_H * GAREA_H / (g_profs.size() * CARD_H);
      int barY = GAREA_TOP + (g_scrollY * (GAREA_H - barH)) / mx;
      RECT bar = {wrc.right - 7, barY, wrc.right - 2, barY + barH};
      DrawRR(mdc, bar, 4, 4, CLR_MUTED, 0, 0);
    }

    SelectObject(mdc, g_fSm);
    SetTextColor(mdc, RGB(50, 50, 75));
    RECT ft = {0, FOOTER_Y, wrc.right, WIN_H};
    DrawText(mdc, L"EzRes v2.0 by ItzCooode  |  No permanent system changes", -1, &ft,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    BitBlt(hdc, 0, 0, wrc.right, wrc.bottom, mdc, 0, 0, SRCCOPY);
    SelectObject(mdc, obmp);
    DeleteObject(mbmp);
    DeleteDC(mdc);
    EndPaint(hWnd, &ps);
    return 0;
  }

  case WM_COMMAND: {
    WORD id = LOWORD(wParam), ntf = HIWORD(wParam);
    if (id == ID_CMB_MONITOR && ntf == CBN_SELCHANGE) {
      int mi = MonSel(hWnd);
      if (mi >= 0 && (size_t)mi < g_mons.size()) {
        EnumModes(g_mons[mi].dev);
        int ai = AspSel(hWnd);
        FilterModes((ai >= 0 && ai < ASPECT_CNT) ? ASPECTS[ai].ratio : 0.0);
        PopRes(hWnd);
      }
    }
    if (id == ID_CMB_ASPECT && ntf == CBN_SELCHANGE) {
      int ai = AspSel(hWnd);
      FilterModes((ai >= 0 && ai < ASPECT_CNT) ? ASPECTS[ai].ratio : 0.0);
      PopRes(hWnd);
    }
    if (id == ID_CMB_RES && ntf == CBN_SELCHANGE)
      PopHz(hWnd);
    return 0;
  }

  case WM_LBUTTONDOWN: {
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    RECT wrc;
    GetClientRect(hWnd, &wrc);

    {
      RECT rc = {wrc.right - 34, 14, wrc.right - 6, 38};
      if (PtInRect(&rc, pt)) {
        DestroyWindow(hWnd);
        return 0;
      }
    }
    {
      RECT rc = {wrc.right - 70, 14, wrc.right - 38, 38};
      if (PtInRect(&rc, pt)) {
        MinimizeToTray(hWnd);
        return 0;
      }
    }

    {
      RECT rc = {20, 348, 250, 394};
      if (PtInRect(&rc, pt)) {
        g_bApply.press = true;
        InvalidateRect(hWnd, nullptr, FALSE);
        ApplyRes(hWnd);
        g_bApply.press = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
      }
    }
    {
      RECT rc = {258, 348, wrc.right - 20, 394};
      if (PtInRect(&rc, pt)) {
        g_bRestore.press = true;
        InvalidateRect(hWnd, nullptr, FALSE);
        RestoreRes(hWnd);
        g_bRestore.press = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
      }
    }

    if (pt.y >= GAREA_TOP && pt.y < GAREA_BOT) {
      for (int i = 0; i < (int)g_profs.size(); ++i) {
        int y = CardY(i);
        if (pt.y < y || pt.y >= y + CARD_H)
          continue;
        auto &p = g_profs[i];

        RECT pb = {wrc.right - 206, y + 18, wrc.right - 110, y + CARD_H - 18};
        RECT lb = {wrc.right - 104, y + 18, wrc.right - 20, y + CARD_H - 18};

        if (PtInRect(&pb, pt) && p.cfgFound && !p.cfgTpl.empty()) {
          DWORD w, h, hz;
          GetSelRes(hWnd, w, h, hz);
          if (!w) {
            SetStat(L"[!] Pick a resolution first!", CLR_WARNING);
            return 0;
          }
          if (DoPatch(p, w, h)) {
            std::wostringstream ss;
            ss << p.name << L" patched to " << w << L"x" << h
               << L". Restart the game!";
            SetStat(ss.str(), CLR_SUCCESS);
          } else
            SetStat(L"[X] Patch failed. Try Run as Administrator.", CLR_DANGER);
          InvalidateRect(hWnd, nullptr, FALSE);
        }
        if (PtInRect(&lb, pt) && p.exeFound) {
          HINSTANCE r = ShellExecute(nullptr, L"open", p.exePath.c_str(),
                                     nullptr, nullptr, SW_SHOWNORMAL);
          if ((INT_PTR)r <= 32)
            SetStat(L"[X] Could not launch game.", CLR_DANGER);
          else
            SetStat(L"[OK] Launching " + p.name + L"...", CLR_SUCCESS);
        }
        break;
      }
      return 0;
    }

    if (pt.y < 68) {
      g_drag = true;
      g_dragPt = pt;
      SetCapture(hWnd);
    }
    return 0;
  }
  case WM_LBUTTONUP:
    g_drag = false;
    ReleaseCapture();
    return 0;

  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    RECT wrc;
    GetClientRect(hWnd, &wrc);
    bool rd = false;

    auto chk = [&](BtnSt &b, int l, int t, int r, int bot) {
      RECT rr = {l, t, r, bot};
      bool h = !!PtInRect(&rr, pt);
      if (h != b.hov) {
        b.hov = h;
        rd = true;
      }
    };
    chk(g_bClose, wrc.right - 34, 14, wrc.right - 6, 38);
    chk(g_bMin, wrc.right - 70, 14, wrc.right - 38, 38);
    chk(g_bApply, 20, 348, 250, 394);
    chk(g_bRestore, 258, 348, wrc.right - 20, 394);

    if (pt.y >= GAREA_TOP && pt.y < GAREA_BOT) {
      for (int i = 0; i < (int)g_profs.size(); ++i) {
        int y = CardY(i);
        auto &p = g_profs[i];
        RECT pb = {wrc.right - 206, y + 18, wrc.right - 110, y + CARD_H - 18};
        RECT lb = {wrc.right - 104, y + 18, wrc.right - 20, y + CARD_H - 18};
        bool hp = !!PtInRect(&pb, pt), hl = !!PtInRect(&lb, pt);
        if (hp != p.hovPatch || hl != p.hovLaunch) {
          p.hovPatch = hp;
          p.hovLaunch = hl;
          rd = true;
        }
      }
    } else {
      for (auto &p : g_profs)
        if (p.hovPatch || p.hovLaunch) {
          p.hovPatch = p.hovLaunch = false;
          rd = true;
        }
    }

    if (rd)
      InvalidateRect(hWnd, nullptr, FALSE);
    if (g_drag) {
      POINT sc;
      GetCursorPos(&sc);
      SetWindowPos(hWnd, nullptr, sc.x - g_dragPt.x, sc.y - g_dragPt.y, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER);
    }
    return 0;
  }

  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
    g_scrollY = max(0, min(MaxScroll(), g_scrollY - delta * CARD_H));
    InvalidateRect(hWnd, nullptr, FALSE);
    return 0;
  }

  case WM_TIMER:
    return 0;

  case WM_TRAYICON:
    if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN) {
      RestoreFromTray(hWnd);
    }
    return 0;

  case 0x0138:
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX: {
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, CLR_CARD);
    SetTextColor(hdc, CLR_TEXT);
    static HBRUSH s_cbBr = nullptr;
    if (!s_cbBr)
      s_cbBr = CreateSolidBrush(CLR_CARD);
    return (LRESULT)s_cbBr;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_DESTROY:
    RemoveTrayIcon();
    if (g_applied) {
      int mi = MonSel(hWnd);
      if (mi >= 0 && (size_t)mi < g_mons.size())
        ChangeDisplaySettingsEx(g_mons[mi].dev.c_str(), &g_saved, nullptr,
                                CDS_FULLSCREEN, nullptr);
    }
    DeleteObject(g_fTitle);
    DeleteObject(g_fBig);
    DeleteObject(g_fMed);
    DeleteObject(g_fSm);
    DeleteObject(g_fTag);
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
  INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&icc);

  g_fTitle = MkFont(22, FW_BOLD, false);
  g_fBig = MkFont(13, FW_SEMIBOLD);
  g_fMed = MkFont(11, FW_NORMAL);
  g_fSm = MkFont(9, FW_NORMAL);
  g_fTag = MkFont(10, FW_BOLD);

  WNDCLASSEX wc{sizeof(wc)};
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = CreateSolidBrush(CLR_BG);
  wc.lpszClassName = L"EzRes";
  wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  wc.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  RegisterClassEx(&wc);

  int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
  g_hWnd =
      CreateWindowEx(WS_EX_APPWINDOW, L"EzRes", L"EzRes",
                     WS_POPUP | WS_VISIBLE, (sw - WIN_W) / 2, (sh - WIN_H) / 2,
                     WIN_W, WIN_H, nullptr, nullptr, hInst, nullptr);

  BOOL dark = TRUE;
  DwmSetWindowAttribute(g_hWnd, 20, &dark, sizeof(dark));

  MSG m{};
  while (GetMessage(&m, nullptr, 0, 0)) {
    TranslateMessage(&m);
    DispatchMessage(&m);
  }
  return (int)m.wParam;
}
