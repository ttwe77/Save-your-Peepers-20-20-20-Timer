// g++ -std=c++17 -mwindows Overlay.cpp -o Overlay.exe -lgdi32 -luser32 -lkernel32 -lwinmm -static -municode -mwindows -O2
#include <windows.h>
#include <string>
#include <map>
#include <fstream>
#include <random>
#include <cctype>
#include <vector>
#include <algorithm>
#include <cstdlib>

// ---------- 简化的 JSON 解析器 ----------
static std::string extractValue(const std::string& json, const std::string& key, bool isString = true) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return "";
    pos++; // skip ':'
    while (pos < json.length() && std::isspace((unsigned char)json[pos])) pos++;
    if (pos >= json.length()) return "";
    if (isString && json[pos] == '\"') {
        pos++; // skip quote
        size_t end = json.find('\"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        // 数字、布尔或 null，不带引号
        size_t end = json.find_first_of(",}\n", pos);
        if (end == std::string::npos) end = json.length();
        std::string val = json.substr(pos, end - pos);
        while (!val.empty() && std::isspace((unsigned char)val.back())) val.pop_back();
        while (!val.empty() && std::isspace((unsigned char)val.front())) val.erase(val.begin());
        return val;
    }
}

static int extractInt(const std::string& json, const std::string& key, int defaultVal) {
    std::string v = extractValue(json, key, false);
    if (v.empty()) return defaultVal;
    try { return std::stoi(v); } catch (...) { return defaultVal; }
}

static bool extractBool(const std::string& json, const std::string& key, bool defaultVal) {
    std::string v = extractValue(json, key, false);
    if (v.empty()) return defaultVal;
    if (v == "true" || v == "1") return true;
    if (v == "false" || v == "0") return false;
    return defaultVal;
}

static std::string extractString(const std::string& json, const std::string& key, const std::string& defaultVal) {
    std::string v = extractValue(json, key, true);
    return v.empty() ? defaultVal : v;
}

// ---------- 翻译表 ----------
struct Translation {
    std::wstring main_text;
    std::wstring time_format;
    std::wstring code_prompt;
};

static std::map<std::string, Translation> translations = {
    {"zh", {L"您已持续用眼过久\n休息一会吧！\n请将注意力集中在至少 6 米远的地方！",
            L"剩余：{} 秒",
            L"请在键盘输入验证码"}},
    {"en", {L"Look away from the screen!",
            L"{} sec left",
            L"Type the code on keyboard"}},
    {"ru", {L"Смотри вдаль!",
            L"Осталось: {} сек.",
            L"Введи код на клавиатуре"}}
};

// ---------- 配置 ----------
struct Config {
    int rest_duration = 20;
    bool require_code = false;
    std::string lang = "zh";
    std::string font_name = "Microsoft YaHei";
};

static Config g_config;

// 新增：控制验证码界面是否显示
static bool g_showCodeInput = false;   // 默认不显示

// ---------- 窗口状态 ----------
static HWND g_hWnd = nullptr;
static HHOOK g_hKeyboardHook = nullptr;   // 低级键盘钩子
static int g_timeLeft = 20;
static bool g_requireCode = false;
static std::string g_code;
static std::string g_typed;
static bool g_codeValid = false;
static double g_alpha = 0.0;
static bool g_fadeComplete = false;

// ---------- 音频状态 ----------
static std::wstring g_lastPlayedSound;
static bool g_hasLastPlayed = false;

const int TIMER_FADE = 1;
const int TIMER_TICK = 2;
const int TIMER_FOCUS = 3;   // 新增：用于抢焦点的定时器

// ---------- 加载配置 ----------
static void LoadConfig() {
    std::ifstream file("settings.json");
    if (!file.is_open()) {
        g_config.rest_duration = 20;
        g_config.require_code = false;
        g_config.lang = "zh";
        g_config.font_name = "Microsoft YaHei";
        return;
    }
    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    g_config.rest_duration = extractInt(json, "rest_duration", 20);
    g_config.require_code = extractBool(json, "require_code", false);
    g_config.lang = extractString(json, "lang", "zh");
    g_config.font_name = extractString(json, "font_name", "Microsoft YaHei");
}

// ---------- 音频辅助函数 ----------
static std::vector<std::wstring> GetSoundFiles(const std::wstring& folder) {
    std::vector<std::wstring> files;
    std::wstring searchPath = folder + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return files;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring name = fd.cFileName;
            size_t dot = name.rfind(L'.');
            if (dot != std::wstring::npos) {
                std::wstring ext = name.substr(dot);
                if (_wcsicmp(ext.c_str(), L".mp3") == 0 ||
                    _wcsicmp(ext.c_str(), L".wav") == 0 ||
                    _wcsicmp(ext.c_str(), L".ogg") == 0) {
                    files.push_back(folder + L"\\" + name);
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    return files;
}

bool PlayRandomSound(bool waitForFinish) {
    // 1. 获取 exe 所在目录，拼接 sounds 文件夹
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::string::npos)
        dir = dir.substr(0, lastSlash + 1);
    std::wstring soundsFolder = dir + L"sounds";

    // 2. 检查文件夹是否存在
    DWORD attr = GetFileAttributesW(soundsFolder.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        return false;

    // 3. 获取所有支持的音频文件
    auto allFiles = GetSoundFiles(soundsFolder);
    if (allFiles.empty())
        return false;

    // 4. 构建候选集（避免连续重复）
    std::vector<std::wstring> candidates = allFiles;
    if (g_hasLastPlayed && allFiles.size() > 1) {
        auto it = std::find(candidates.begin(), candidates.end(), g_lastPlayedSound);
        if (it != candidates.end())
            candidates.erase(it);
    }
    if (candidates.empty())
        candidates = allFiles;

    // 5. 随机选一个
    int idx = rand() % candidates.size();
    std::wstring selected = candidates[idx];
    g_lastPlayedSound = selected;
    g_hasLastPlayed = true;

    // 6. 构建 audio_player.exe 的完整路径
    std::wstring playerPath = dir + L"audio_player.exe";
    if (GetFileAttributesW(playerPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return false;

    // 7. 构建命令行，使用双引号包裹路径（防止空格问题）
    std::wstring cmdLine = L"\"" + playerPath + L"\" \"" + selected + L"\"";

    // 8. 启动进程，不等待（忽略 waitForFinish 参数）
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(
        nullptr,               // 应用程序名
        &cmdLine[0],           // 命令行（可修改）
        nullptr, nullptr,
        FALSE,
        CREATE_NO_WINDOW,      // 不显示控制台窗口（若 audio_player.exe 为 GUI 可去掉）
        nullptr, nullptr,
        &si, &pi
    )) {
        return false;
    }

    // 关闭句柄，不等待进程结束
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// ---------- 生成验证码 ----------
static void GenerateCode() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 9);
    g_code.clear();
    for (int i = 0; i < 4; ++i) g_code.push_back('0' + dis(gen));
    g_typed.clear();
    g_codeValid = false;
}

// ---------- DPI 感知 ----------
static void EnableDPIAwareness() {
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32) {
        auto pSetProcessDPIAwarenessContext = (BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT))GetProcAddress(hUser32, "SetProcessDPIAwarenessContext");
        if (pSetProcessDPIAwarenessContext) {
            pSetProcessDPIAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            FreeLibrary(hUser32);
            return;
        }
        FreeLibrary(hUser32);
    }
    SetProcessDPIAware();  // 回退
}

// ---------- 创建缩放字体 ----------
static HFONT CreateScaledFont(int pointSize, const std::string& faceName) {
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);
    int height = -MulDiv(pointSize, dpi, 72);
    std::wstring wFace(faceName.begin(), faceName.end());
    return CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, wFace.c_str());
}

// ---------- 低级键盘钩子 ----------
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (down) {
            bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
            bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
            bool win   = ((GetAsyncKeyState(VK_LWIN)   & 0x8000) != 0) ||
                         ((GetAsyncKeyState(VK_RWIN)   & 0x8000) != 0);

            // 1) 单独拦截 Win 键（开始菜单）
            if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN)
                return 1;

            // 2) 修饰键本身放行（不拦截单独的 Ctrl/Alt/Shift）
            bool isModifier = (p->vkCode == VK_CONTROL  ||
                               p->vkCode == VK_LCONTROL ||
                               p->vkCode == VK_RCONTROL ||
                               p->vkCode == VK_MENU     ||
                               p->vkCode == VK_LMENU    ||
                               p->vkCode == VK_RMENU    ||
                               p->vkCode == VK_LSHIFT   ||
                               p->vkCode == VK_RSHIFT);
            if (isModifier)
                return CallNextHookEx(nullptr, nCode, wParam, lParam);

            // 3) 拦截所有带 Ctrl/Alt/Shift/Win 的组合键
            //    例如：Alt+Tab、Alt+F4、Ctrl+Esc、Ctrl+Shift+Esc、Win+... 等
            if (ctrl || alt || shift || win)
                return 1;
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// ---------- 窗口过程 ----------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 设置分层窗口
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, 0, (BYTE)(g_alpha * 255), LWA_ALPHA);
        // 启动淡入定时器
        SetTimer(hWnd, TIMER_FADE, 30, nullptr);
        // 倒计时定时器（1 秒）
        SetTimer(hWnd, TIMER_TICK, 1000, nullptr);
        // 启动持久焦点定时器
        SetTimer(hWnd, TIMER_FOCUS, 50, nullptr);
        // 强制获得焦点并前置
        SetForegroundWindow(hWnd);
        BringWindowToTop(hWnd);
        SetFocus(hWnd);
        break;
    }
    case WM_TIMER: {
        if (wParam == TIMER_FADE) {
            g_alpha += 0.05;
            if (g_alpha >= 0.5) {
                g_alpha = 0.5;
                KillTimer(hWnd, TIMER_FADE);
                g_fadeComplete = true;
            }
            SetLayeredWindowAttributes(hWnd, 0, (BYTE)(g_alpha * 255), LWA_ALPHA);
            InvalidateRect(hWnd, nullptr, TRUE);
        } else if (wParam == TIMER_TICK) {
            if (g_timeLeft > 0) {
                --g_timeLeft;
                InvalidateRect(hWnd, nullptr, TRUE);
                if (g_timeLeft == 0) {
                    PostQuitMessage(0);  // 正常结束
                }
            }
        } else if (wParam == TIMER_FOCUS) {
            // 强制窗口保持在最前
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            BringWindowToTop(hWnd);
            SetForegroundWindow(hWnd);
            SetFocus(hWnd);
            
            // 更激进：附加输入线程以绕过前台锁定
            HWND hFore = GetForegroundWindow();
            if (hFore && hFore != hWnd) {
                DWORD foreThread = GetWindowThreadProcessId(hFore, nullptr);
                DWORD myThread = GetCurrentThreadId();
                if (foreThread != myThread) {
                    AttachThreadInput(foreThread, myThread, TRUE);
                    SetForegroundWindow(hWnd);
                    SetFocus(hWnd);
                    AttachThreadInput(foreThread, myThread, FALSE);
                }
            }
        }
        break;
    }
    case WM_ACTIVATE:
        // 若窗口被切到后台，立即抢回焦点
        if (LOWORD(wParam) == WA_INACTIVE) {
            SetForegroundWindow(hWnd);
            SetFocus(hWnd);
            // 可选：刷新置顶状态
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);

        // 黑色背景
        HBRUSH hBrush = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        SetTextColor(hdc, RGB(255,255,255));
        SetBkMode(hdc, TRANSPARENT);

        // 获取翻译
        auto it = translations.find(g_config.lang);
        if (it == translations.end()) it = translations.find("en");
        const Translation& tr = it->second;

        if (g_requireCode && g_showCodeInput && !g_codeValid) {
            // ---- 验证码模式 ----
            // 1. 倒计时（固定在最底部，高度 50）
            RECT timeRect = rect;
            timeRect.top = rect.bottom - 50;
            timeRect.bottom = rect.bottom;

            // 2. 圆点进度 + 提示（拆分为两个区域，分别绘制，避免重叠）
            RECT progressRect = rect;
            progressRect.top = rect.bottom - 140;   // 圆点区域顶部
            progressRect.bottom = rect.bottom - 100; // 圆点区域底部

            RECT promptRect = rect;
            promptRect.top = rect.bottom - 100;    // 提示区域顶部
            promptRect.bottom = rect.bottom - 60;  // 提示区域底部（高度 40）

            // 3. 验证码数字
            RECT codeRect = rect;
            codeRect.bottom = rect.bottom - 150;  // 与提示区域留出 10 像素间隔

            // 绘制倒计时
            SelectObject(hdc, CreateScaledFont(24, g_config.font_name));
            std::wstring timeStr = tr.time_format;
            size_t pos = timeStr.find(L"{}");
            if (pos != std::wstring::npos)
                timeStr.replace(pos, 2, std::to_wstring(g_timeLeft));
            DrawTextW(hdc, timeStr.c_str(), -1, &timeRect, DT_CENTER | DT_VCENTER | DT_NOCLIP);

            // 绘制圆点进度（单行显示）
            std::wstring progress;
            for (size_t i = 0; i < g_typed.size(); ++i) progress.push_back(L'●');
            for (size_t i = g_typed.size(); i < g_code.size(); ++i) progress.push_back(L'○');
            DrawTextW(hdc, progress.c_str(), -1, &progressRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

            // 绘制提示文字（单行显示）
            std::wstring promptText(tr.code_prompt.begin(), tr.code_prompt.end());
            DrawTextW(hdc, promptText.c_str(), -1, &promptRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

            // 绘制验证码数字
            HFONT hFontCode = CreateScaledFont(60, g_config.font_name);
            HFONT hOld = (HFONT)SelectObject(hdc, hFontCode);
            DrawTextW(hdc, std::wstring(g_code.begin(), g_code.end()).c_str(), -1, &codeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

            DeleteObject(SelectObject(hdc, hOld));
            DeleteObject(hFontCode);
        } else {
            // ---- 普通休息模式 ----
            // 对应 translations[0]: 主文本 (tr.main_text)
            HFONT hFontMain = CreateScaledFont(36, g_config.font_name);
            HFONT hOld = (HFONT)SelectObject(hdc, hFontMain);
            
            // 先计算文本所需高度（排除底部倒计时区域）
            RECT calcRect = rect;
            calcRect.bottom = rect.bottom - 60;   // 预留倒计时空间
            DrawTextW(hdc, tr.main_text.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER | DT_WORDBREAK | DT_NOCLIP);
            int textHeight = calcRect.bottom - calcRect.top;
            
            // 在预留区域内垂直居中
            RECT mainRect = rect;
            mainRect.bottom = rect.bottom - 60;
            mainRect.top = (mainRect.bottom - mainRect.top - textHeight) / 2;
            mainRect.bottom = mainRect.top + textHeight;
            
            DrawTextW(hdc, tr.main_text.c_str(), -1, &mainRect, DT_CENTER | DT_WORDBREAK | DT_NOCLIP);

            // 2. 倒计时 (对应 translations[2]: tr.time_format)
            SelectObject(hdc, CreateScaledFont(24, g_config.font_name));
            std::wstring timeStr = tr.time_format;
            size_t pos = timeStr.find(L"{}");
            if (pos != std::wstring::npos)
                timeStr.replace(pos, 2, std::to_wstring(g_timeLeft));
            RECT timeRect = rect;
            timeRect.top = rect.bottom - 50;
            timeRect.bottom = rect.bottom;
            DrawTextW(hdc, timeStr.c_str(), -1, &timeRect, DT_CENTER | DT_NOCLIP);

            DeleteObject(SelectObject(hdc, hOld));
            DeleteObject(hFontMain);
        }

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_KEYDOWN: {
        if (g_requireCode && g_showCodeInput && !g_codeValid) {
            // 数字键处理
            if ( (wParam >= '0' && wParam <= '9') || (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) ) {
                char ch;
                if (wParam >= '0' && wParam <= '9')
                    ch = (char)wParam;
                else
                    ch = '0' + (wParam - VK_NUMPAD0);

                if (g_typed.size() < g_code.size()) {
                    if (ch == g_code[g_typed.size()]) {
                        g_typed.push_back(ch);
                        InvalidateRect(hWnd, nullptr, TRUE);
                        if (g_typed == g_code) {
                            g_codeValid = true;
                            PostQuitMessage(0);
                        }
                    } else {
                        g_typed.clear();
                        InvalidateRect(hWnd, nullptr, TRUE);
                    }
                }
            } else if (wParam == VK_BACK) {
                if (!g_typed.empty()) {
                    g_typed.pop_back();
                    InvalidateRect(hWnd, nullptr, TRUE);
                }
            }
        } else {
            // 非验证码模式或已验证
            if (wParam == VK_ESCAPE) {
                if (g_requireCode) {
                    // 进入验证码模式
                    g_showCodeInput = true;
                    GenerateCode();
                    InvalidateRect(hWnd, nullptr, TRUE);
                } else {
                    // 直接结束休息
                    PostQuitMessage(0);
                }
            }
        }
        break;
    }
    case WM_SYSCOMMAND: {
        if ((wParam & 0xFFF0) == SC_CLOSE) {
            if (g_requireCode) {
                g_showCodeInput = true;
                GenerateCode();
                InvalidateRect(hWnd, nullptr, TRUE);
                return 0;   // 阻止窗口关闭
            }
        }
        break;
    }
    case WM_DESTROY: {
        KillTimer(hWnd, TIMER_FOCUS);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------- 主函数 ----------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    EnableDPIAwareness();
    LoadConfig();
    srand(GetTickCount());

    g_timeLeft = g_config.rest_duration;
    g_requireCode = g_config.require_code;

    // 注册窗口类
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"OverlayClass";
    if (!RegisterClassExW(&wc)) return 1;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 修改：移除 WS_EX_NOACTIVATE，允许窗口激活以接收键盘输入
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,  // 不再包含 WS_EX_NOACTIVATE
        L"OverlayClass",
        L"Overlay",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!g_hWnd) return 1;

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);
    PlayRandomSound(false);

    // 安装低级键盘钩子（全局）
    g_hKeyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandleW(nullptr),
        0
    );

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ========== 修改开始 ==========
    // 1. 卸载键盘钩子（窗口已关闭，不再需要拦截）
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = nullptr;
    }

    // 2. 销毁窗口（使其立即从屏幕消失）
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }

    // 3. 播放音频并等待播放完毕（此时窗口已消失）
    PlayRandomSound(true);

    // 定时器会在窗口销毁时自动清理，无需手动 KillTimer
    // ========== 修改结束 ==========

    return 0;

    return 0;
}