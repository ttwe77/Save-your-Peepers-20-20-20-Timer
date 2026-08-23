// securedesktop.cpp — 安全桌面版（多语言支持 + 壁纸背景 + 独立提示窗口 + 随机音频播放）
// 编译:
// windres resource.rc -O coff -o resource.res
// g++ -o securedesktop.exe securedesktop.cpp resource.res -lcomctl32 -lgdi32 -luser32 -ladvapi32 -lshlwapi -lcomdlg32 -lgdiplus -lwinmm -static -mwindows -O2 -ldwmapi

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <dwmapi.h>   // 需要链接 dwmapi.lib
#include <shellscalingapi.h>
#include <gdiplus.h>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <algorithm>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20   // Windows 10 20H1+；旧系统可改为 19
#endif

using namespace Gdiplus;

// ========== 全局变量 ==========
static HWND g_hBgWnd = nullptr;
static HWND g_hPromptWnd = nullptr;
static HWND g_hTimeLabel = nullptr;
static HWND g_hCodeLabel = nullptr;
static HWND g_hEdit = nullptr;
static HWND g_hBtn = nullptr;
static int g_seconds = 20;
static std::wstring g_code = L"none";
static bool g_codeRequired = false;

static WNDPROC g_oldEditProc = nullptr;
static WNDPROC g_oldBtnProc  = nullptr;

static HDESK     g_hOriginal = nullptr;
static HDESK     g_hNew      = nullptr;
static int       g_ScreenW   = 0;
static int       g_ScreenH   = 0;
static Image*    g_pWallpaper = nullptr;
static bool      g_hasWallpaper = false;



// 深色模式颜色
static HBRUSH  g_hDarkBrush = nullptr;
static COLORREF g_darkBg   = RGB(32, 32, 32);
static COLORREF g_darkText = RGB(240, 240, 240);

// ---------- 多语言相关 ----------
static std::wstring g_lang = L"zh";
static std::wstring g_title;
static std::wstring g_mainPrompt;
static std::wstring g_codePrefix;
static std::wstring g_skipButton;
static std::wstring g_errorMsg;

static void setup_language_strings() {
    if (g_lang == L"en") {
        g_title       = L"Save Your Peepers 👁️";
        g_mainPrompt  = L"Look away!";
        g_codePrefix  = L"Code: ";
        g_skipButton  = L"Skip Cycle";
        g_errorMsg    = L"Wrong code, please retry!";
    } else if (g_lang == L"ru") {
        g_title       = L"Бережем глазки 👁️";
        g_mainPrompt  = L"Смотри вдаль!";
        g_codePrefix  = L"Код: ";
        g_skipButton  = L"Скипнуть цикл";
        g_errorMsg    = L"Неверный код, попробуйте снова!";
    } else { // 默认中文
        g_title       = L"守护双眼👁️";
        g_mainPrompt  = L"您已持续用眼过久,休息一会吧！\n请将注意力集中在至少6米远的地方！";
        g_codePrefix  = L"验证码：";
        g_skipButton  = L"跳过本轮";
        g_errorMsg    = L"验证码错误，请重新输入！";
    }
}

// ---------- 简易 JSON 解析 ----------
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

static bool parse_json_value(const std::string& json, const std::string& key, std::string& out_value) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;
    size_t start = json.find_first_not_of(" \t\n\r", pos + 1);
    if (start == std::string::npos) return false;
    size_t end = start;
    if (json[start] == '"') {
        start++;
        end = json.find('"', start);
        if (end == std::string::npos) return false;
        out_value = json.substr(start, end - start);
        return true;
    } else {
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n' && json[end] != '\r')
            end++;
        out_value = trim(json.substr(start, end - start));
        return true;
    }
}

static void load_settings() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::string::npos)
        dir = dir.substr(0, lastSlash + 1);
    std::wstring configPath = dir + L"settings.json";

    std::ifstream file(configPath.c_str());
    if (!file.is_open()) {
        // 默认中文
        setup_language_strings();
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    std::string val;
    if (parse_json_value(json, "rest_duration", val)) {
        int sec = std::atoi(val.c_str());
        if (sec > 0) g_seconds = sec;
    }
    if (parse_json_value(json, "require_code", val)) {
        if (val == "true") {
            g_codeRequired = true;
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            wchar_t code[16];
            for (int i = 0; i < 4; ++i)
                code[i] = L'0' + (std::rand() % 10);
            code[4] = L'\0';
            g_code = code;
        }
    }

    // 读取语言设置
    if (parse_json_value(json, "lang", val)) {
        if (val == "en") g_lang = L"en";
        else if (val == "ru") g_lang = L"ru";
        else g_lang = L"zh";
    } else {
        g_lang = L"zh";
    }

    setup_language_strings();
}

// ========== 安全桌面相关 ==========
void SetDpiAwareness() {
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        typedef HRESULT (WINAPI *SetProcessDpiAwareness_t)(int);
        auto pSetProcessDpiAwareness = (SetProcessDpiAwareness_t)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSetProcessDpiAwareness) {
            HRESULT hr = pSetProcessDpiAwareness(2);
            FreeLibrary(hShcore);
            if (SUCCEEDED(hr)) return;
        } else {
            FreeLibrary(hShcore);
        }
    }
    SetProcessDPIAware();
}

void CreateSecureDesktop() {
    g_hOriginal = OpenInputDesktop(0, FALSE, DESKTOP_SWITCHDESKTOP);
    if (!g_hOriginal) {
        DWORD tid = GetCurrentThreadId();
        g_hOriginal = GetThreadDesktop(tid);
    }
    if (!g_hOriginal)
        throw std::runtime_error("无法获取当前桌面句柄");

    g_hNew = CreateDesktopW(L"RestReminder", nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (!g_hNew)
        throw std::runtime_error("创建桌面失败");

    if (!SetThreadDesktop(g_hNew)) {
        DWORD err = GetLastError();
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
        throw std::runtime_error("SetThreadDesktop 失败");
    }

    if (!SwitchDesktop(g_hNew)) {
        DWORD err = GetLastError();
        SetThreadDesktop(g_hOriginal);
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
        throw std::runtime_error("SwitchDesktop 失败");
    }
}

void RestoreDesktop() {
    if (g_hOriginal) {
        SetThreadDesktop(g_hOriginal);
        SwitchDesktop(g_hOriginal);
    }
    if (g_hNew) {
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
    }
}

// ========== 壁纸加载 ==========
std::wstring GetWallpaperPath() {
    WCHAR buf[MAX_PATH] = {0};
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, buf, 0)) {
        if (GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES)
            return buf;
    }
    return L"";
}

void LoadWallpaper() {
    std::wstring path = GetWallpaperPath();
    if (!path.empty()) {
        g_pWallpaper = Image::FromFile(path.c_str());
        if (g_pWallpaper && g_pWallpaper->GetLastStatus() == Ok) {
            g_hasWallpaper = true;
            return;
        }
        delete g_pWallpaper;
        g_pWallpaper = nullptr;
    }
    g_hasWallpaper = false;
}

// 启动 audio_player.exe（不等待）
static void LaunchAudioPlayer() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::string::npos)
        dir = dir.substr(0, lastSlash + 1);
    std::wstring playerPath = dir + L"audio_player.exe";

    // 检查文件是否存在
    if (GetFileAttributesW(playerPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;   // 静默忽略，无音频播放

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(playerPath.c_str(), NULL, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// ========== 背景窗口过程 ==========
LRESULT CALLBACK BgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (g_hasWallpaper && g_pWallpaper) {
            Graphics graphics(hdc);
            graphics.DrawImage(g_pWallpaper, 0, 0, rc.right, rc.bottom);
        } else {
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ========== 子类窗口过程 ==========
LRESULT CALLBACK SubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        HWND hParent = GetParent(hWnd);
        if (hParent) SendMessage(hParent, msg, wParam, lParam);
        return 0;
    }
    if (msg == WM_SYSKEYDOWN && wParam == VK_F4 && (lParam & (1 << 29))) {
        HWND hParent = GetParent(hWnd);
        if (hParent) SendMessage(hParent, msg, wParam, lParam);
        return 0;
    }
    return CallWindowProc(oldProc, hWnd, msg, wParam, lParam);
}

// ========== 提示窗口过程 ==========
LRESULT CALLBACK PromptWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFontBig = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HFONT hFontSmall = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HFONT hFontBtn = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

        // 时间标签
        wchar_t timeStr[16];
        wsprintfW(timeStr, L"%d", g_seconds);
        g_hTimeLabel = CreateWindowW(L"STATIC", timeStr,
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            50, 20, 300, 80, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        SendMessage(g_hTimeLabel, WM_SETFONT, (WPARAM)hFontBig, TRUE);

        // 主提示（多语言）
        HWND hHint = CreateWindowW(L"STATIC", g_mainPrompt.c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            50, 110, 320, 50, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        SendMessage(hHint, WM_SETFONT, (WPARAM)hFontSmall, TRUE);

        if (g_codeRequired) {
            // 验证码标签（多语言前缀）
            std::wstring codeText = g_codePrefix + g_code;
            g_hCodeLabel = CreateWindowW(L"STATIC", codeText.c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                50, 160, 300, 30, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_hCodeLabel, WM_SETFONT, (WPARAM)hFontSmall, TRUE);

            g_hEdit = CreateWindowW(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
                100, 195, 200, 30, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_hEdit, WM_SETFONT, (WPARAM)hFontSmall, TRUE);

            g_hBtn = CreateWindowW(L"BUTTON", g_skipButton.c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                150, 250, 120, 40, hWnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_hBtn, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        } else {
            g_hBtn = CreateWindowW(L"BUTTON", g_skipButton.c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                150, 190, 120, 40, hWnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_hBtn, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        }

        SetTimer(hWnd, 1, 1000, nullptr);

        if (g_hEdit) {
            g_oldEditProc = (WNDPROC)SetWindowLongPtr(g_hEdit, GWLP_WNDPROC, (LONG_PTR)SubclassProc);
            SetWindowLongPtr(g_hEdit, GWLP_USERDATA, (LONG_PTR)g_oldEditProc);
        }
        if (g_hBtn) {
            g_oldBtnProc = (WNDPROC)SetWindowLongPtr(g_hBtn, GWLP_WNDPROC, (LONG_PTR)SubclassProc);
            SetWindowLongPtr(g_hBtn, GWLP_USERDATA, (LONG_PTR)g_oldBtnProc);
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, g_darkBg);
        SetTextColor(hdc, g_darkText);
        return (LRESULT)g_hDarkBrush;
    }

    case WM_TIMER:
        if (--g_seconds <= 0) {
            KillTimer(hWnd, 1);
            PostQuitMessage(0);
        } else {
            wchar_t buf[16];
            wsprintf(buf, L"%d", g_seconds);
            SetWindowTextW(g_hTimeLabel, buf);
        }
        return 0;

    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            if (!g_codeRequired) {
                PostQuitMessage(0);
            } else {
                wchar_t input[32];
                GetWindowTextW(g_hEdit, input, 32);
                if (wcscmp(input, g_code.c_str()) == 0) {
                    PostQuitMessage(0);
                } else {
                    SetWindowTextW(g_hEdit, L"");
                    MessageBoxW(hWnd, g_errorMsg.c_str(), L"错误", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }

        if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == g_hEdit) {
            if (g_codeRequired) {
                wchar_t input[32];
                GetWindowTextW(g_hEdit, input, 32);
                if (wcscmp(input, g_code.c_str()) == 0) {
                    PostQuitMessage(0);
                }
            }
            return 0;
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && !g_codeRequired) {
            PostQuitMessage(0);
        }
        return 0;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4 && (lParam & (1 << 29))) {
            if (!g_codeRequired) {
                PostQuitMessage(0);
                return 0;
            }
            return 0;
        }
        break;

    case WM_CLOSE:
        if (!g_codeRequired) {
            PostQuitMessage(0);
            return 0;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ========== 创建窗口 ==========
void CreateWindows(HINSTANCE hInstance) {
    const wchar_t BG_CLASS[] = L"BgWndClass";
    WNDCLASSW wcBg = {};
    wcBg.lpfnWndProc   = BgWndProc;
    wcBg.hInstance     = hInstance;
    wcBg.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcBg.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcBg.lpszClassName = BG_CLASS;
    wcBg.hIcon = LoadIconW(hInstance, L"MAINICON");
    RegisterClassW(&wcBg);

    const wchar_t PROMPT_CLASS[] = L"PromptWndClass";
    WNDCLASSW wcPrompt = {};
    wcPrompt.lpfnWndProc   = PromptWndProc;
    wcPrompt.hInstance     = hInstance;
    wcPrompt.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcPrompt.hbrBackground = g_hDarkBrush;   // 深色背景画刷
    wcPrompt.lpszClassName = PROMPT_CLASS;
    wcPrompt.hIcon = LoadIconW(hInstance, L"MAINICON");
    RegisterClassW(&wcPrompt);

    g_ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenH = GetSystemMetrics(SM_CYSCREEN);

    g_hBgWnd = CreateWindowExW(
        0,
        BG_CLASS,
        L"壁纸背景",
        WS_POPUP,
        0, 0, g_ScreenW, g_ScreenH,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!g_hBgWnd) {
        throw std::runtime_error("创建背景窗口失败");
    }

    int clientWidth = 400;
    int clientHeight = g_codeRequired ? 320 : 270;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX;
    DWORD dwExStyle = WS_EX_TOPMOST;
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;
    int posX = (g_ScreenW - winWidth) / 2;
    int posY = (g_ScreenH - winHeight) / 2;

    // 使用多语言标题
    g_hPromptWnd = CreateWindowExW(
        dwExStyle,
        PROMPT_CLASS,
        g_title.c_str(),          // 窗口标题（多语言）
        dwStyle,
        posX, posY, winWidth, winHeight,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!g_hPromptWnd) {
        throw std::runtime_error("创建提示窗口失败");
    }

    // ===== 新增：启用深色标题栏 =====
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(g_hPromptWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    // ===============================

    ShowWindow(g_hBgWnd, SW_SHOW);
    UpdateWindow(g_hBgWnd);
    ShowWindow(g_hPromptWnd, SW_SHOW);
    UpdateWindow(g_hPromptWnd);
    SetForegroundWindow(g_hPromptWnd);
    SetWindowPos(g_hPromptWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (g_codeRequired && g_hEdit) {
        SetFocus(g_hEdit);
        SendMessage(g_hEdit, EM_SETSEL, 0, -1);
    } else {
        SetFocus(g_hPromptWnd);
    }
}

// ========== 主入口 ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetDpiAwareness();

    ULONG_PTR gdiplusToken = 0;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    load_settings();  // 会调用 setup_language_strings()

    try {
        CreateSecureDesktop();
    } catch (const std::exception&) {
        // 启动同目录下的 Overlay.exe
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring dir = exePath;
        size_t lastSlash = dir.find_last_of(L"\\/");
        if (lastSlash != std::string::npos)
            dir = dir.substr(0, lastSlash + 1);
        std::wstring overlayPath = dir + L"Overlay.exe";

        if (GetFileAttributesW(overlayPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            if (CreateProcessW(overlayPath.c_str(), NULL, NULL, NULL, FALSE,
                               CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        GdiplusShutdown(gdiplusToken);
        return 0;   // 退出自身
    }

    // 桌面创建成功，继续执行
    LaunchAudioPlayer();

    LoadWallpaper();
    
    // 创建深色画刷
    g_hDarkBrush = CreateSolidBrush(g_darkBg);

    try {
        CreateWindows(hInstance);
    } catch (const std::exception& e) {
        WCHAR buf[512];
        swprintf(buf, 512, L"创建窗口失败: %hs", e.what());
        MessageBoxW(nullptr, buf, L"错误", MB_OK | MB_ICONWARNING);
        RestoreDesktop();
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 退出清理
    if (g_hPromptWnd) {
        DestroyWindow(g_hPromptWnd);
        g_hPromptWnd = nullptr;
    }
    if (g_hBgWnd) {
        DestroyWindow(g_hBgWnd);
        g_hBgWnd = nullptr;
    }
    if (g_hDarkBrush) {
        DeleteObject(g_hDarkBrush);
        g_hDarkBrush = nullptr;
    }

    RestoreDesktop();
    LaunchAudioPlayer();

    delete g_pWallpaper;
    GdiplusShutdown(gdiplusToken);
    return 0;
}