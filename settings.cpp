// settings.cpp
// 编译：g++ -o Settings.exe settings.cpp -lcomctl32 -lgdi32 -luser32 -ladvapi32 -lshlwapi -static -municode -mwindows -O2 -lcomdlg32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <set>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comdlg32.lib")

// 将 UTF-8 字节串转为宽字符串
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::vector<wchar_t> buffer(len);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buffer.data(), len);
    return std::wstring(buffer.data());
}

// 全局变量前向声明（供 HotkeyCaptureWndProc 使用）
extern HFONT g_hUIFont;

// 函数前向声明（供 HotkeyCaptureWndProc 使用）
void ApplyTitleBarTheme(HWND hwnd);

// 全局标志：标记热键对话框是否正在活动
bool g_hotkeyDlgActive = false;

// 热键捕获窗口过程
LRESULT CALLBACK HotkeyCaptureWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND s_hTargetEdit = NULL;  // 目标编辑框句柄
    static std::wstring s_hotkeyText;  // 当前捕获的组合键字符串

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            s_hTargetEdit = (HWND)pCS->lpCreateParams;
            s_hotkeyText.clear();
            SetWindowTextW(hWnd, L"按下组合键...");
            // 居中显示
            RECT rcWnd, rcParent;
            GetWindowRect(hWnd, &rcWnd);
            GetWindowRect(GetParent(hWnd), &rcParent);
            int x = rcParent.left + (rcParent.right - rcParent.left - (rcWnd.right - rcWnd.left)) / 2;
            int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcWnd.bottom - rcWnd.top)) / 2;
            SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            // 应用字体和主题
            SendMessage(hWnd, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
            ApplyTitleBarTheme(hWnd);
            // 捕获键盘输入
            SetFocus(hWnd);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            // 支持 Esc 取消
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }

            // 忽略单独的修饰键
            if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU ||
                wParam == VK_LWIN   || wParam == VK_RWIN) {
                return 0;
            }

            bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
            bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;

            if (!ctrl && !shift && !alt) return 0;

            // 构建修饰键部分
            std::wstring mods;
            if (ctrl)  mods += L"Ctrl+";
            if (shift) mods += L"Shift+";
            if (alt)   mods += L"Alt+";

            std::wstring keyName;

            // 常见字母/数字/功能键，保持大写统一
            if (wParam >= 'A' && wParam <= 'Z') {
                keyName = (wchar_t)wParam; // 大写
            } else if (wParam >= '0' && wParam <= '9') {
                keyName = (wchar_t)wParam;
            } else if (wParam >= VK_F1 && wParam <= VK_F24) {
                keyName = L"F" + std::to_wstring(wParam - VK_F1 + 1);
            } else {
                // 其他键：用系统键名
                wchar_t nameBuf[128] = {0};
                // 保留扫描码 + 扩展键标志（bit 24）
                LONG lParamScan = (LONG)(lParam & 0x01FF0000);
                if (GetKeyNameTextW(lParamScan, nameBuf, 128) > 0) {
                    keyName = nameBuf;
                } else {
                    return 0; // 无法识别的键，忽略
                }
            }

            s_hotkeyText = mods + keyName;
            SetWindowTextW(hWnd, s_hotkeyText.c_str());

            // 避免重复按键产生多个定时器
            KillTimer(hWnd, 1);
            SetTimer(hWnd, 1, 300, NULL);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                KillTimer(hWnd, 1);
                // 将结果写入目标编辑框
                if (s_hTargetEdit && !s_hotkeyText.empty()) {
                    SetWindowTextW(s_hTargetEdit, s_hotkeyText.c_str());
                }
                DestroyWindow(hWnd);
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            g_hotkeyDlgActive = false;
            return 0;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

// 显示热键捕获对话框（模态）
void ShowHotkeyCaptureDialog(HWND hParent, HWND hTargetEdit) {
    // 注册窗口类（可只在第一次调用时注册）
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = HotkeyCaptureWndProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"HotkeyCaptureClass";
        if (!RegisterClassExW(&wc)) return;
        classRegistered = true;
    }

    // 创建弹出窗口（模态） 
    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"HotkeyCaptureClass",
        L"按下组合键...",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 110,
        hParent, NULL, GetModuleHandle(NULL), (LPVOID)hTargetEdit
    );

    if (!hWnd) return;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // 进入模态消息循环（禁用父窗口）
    EnableWindow(hParent, FALSE);
    g_hotkeyDlgActive = true;

    MSG msg;
    while (g_hotkeyDlgActive) {
        if (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
}

// 设置结构
struct Settings {
    std::wstring lang = L"zh";
    int work_duration = 1200;       // 秒
    int rest_duration = 20;         // 秒
    bool use_overlay = true;
    bool require_code = true;
    bool use_warning = true;
    std::wstring warn_position = L"pos_br";
    std::wstring hotkey_pause = L"ctrl+shift+p";
    std::wstring hotkey_skip = L"ctrl+shift+s";
    std::wstring theme = L"Dark";
    std::wstring font_name = L"Microsoft YaHei UI";
    bool auto_start = false;
    std::wstring overlay_mode = L"fullscreen";
};

Settings g_settings;
bool g_settings_loaded = false;

HFONT g_hUIFont = NULL;
HBRUSH g_hBrushDark = NULL;
HBRUSH g_hBrushLight = NULL;
HBRUSH g_hBrushDarkCtrl = NULL;
HBRUSH g_hBrushLightCtrl = NULL;

// 新增颜色常量
#define COLOR_DARK_BG      RGB(32, 32, 32)
#define COLOR_DARK_CTRL    RGB(45, 45, 45)
#define COLOR_DARK_TEXT    RGB(230, 230, 230)
#define COLOR_DARK_BORDER  RGB(70, 70, 70)
#define COLOR_LIGHT_BG     RGB(245, 245, 245)
#define COLOR_LIGHT_CTRL   RGB(255, 255, 255)
#define COLOR_LIGHT_TEXT   RGB(30, 30, 30)
#define COLOR_LIGHT_BORDER RGB(200, 200, 200)
#define COLOR_ACCENT       RGB(0, 120, 215)
#define COLOR_ACCENT_HOVER RGB(0, 140, 240)
#define COLOR_ACCENT_PRESS RGB(0, 100, 190)

// 应用标题栏主题（深色/浅色），适用于主窗口和字体选择对话框
void ApplyTitleBarTheme(HWND hwnd)
{
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (!hDwm) return;

    typedef HRESULT (WINAPI *DwmSetWindowAttributeFunc)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFunc pDwmSetWindowAttribute =
        (DwmSetWindowAttributeFunc)GetProcAddress(hDwm, "DwmSetWindowAttribute");

    if (pDwmSetWindowAttribute)
    {
        BOOL dark = (g_settings.theme == L"Dark") ? TRUE : FALSE;
        // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Windows 10 1809+)
        // 旧版本为 19，若 20 失败则尝试 19
        if (FAILED(pDwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark))))
        {
            pDwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
        }
    }
    FreeLibrary(hDwm);
}

// 字体选择对话框的钩子过程：让对话框支持深色/浅色主题
UINT_PTR CALLBACK FontDialogHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            // 设置对话框及子控件字体，保持与主界面一致
            if (g_hUIFont)
            {
                SendMessage(hdlg, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                EnumChildWindows(hdlg, [](HWND hChild, LPARAM lParam) -> BOOL {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                    return TRUE;
                }, 0);
            }
            // 根据当前主题设置标题栏颜色（深色/浅色都正确）
            ApplyTitleBarTheme(hdlg);
            return 0;
        }

        case WM_ERASEBKGND:
        {
            // 绘制对话框背景
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hdlg, &rc);
            HBRUSH hBrush = (g_settings.theme == L"Dark") ? g_hBrushDark : g_hBrushLight;
            FillRect(hdc, &rc, hBrush);
            return TRUE;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        {
            // 设置子控件文字颜色和背景色
            HDC hdc = (HDC)wParam;
            bool isDark = (g_settings.theme == L"Dark");
            SetTextColor(hdc, isDark ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT);
            SetBkColor(hdc, isDark ? COLOR_DARK_BG : COLOR_LIGHT_BG);
            return (UINT_PTR)(isDark ? g_hBrushDark : g_hBrushLight);
        }

        default:
            return 0;
    }
}

// 控件 ID
#define IDC_LANG            1001
#define IDC_THEME           1002
#define IDC_FONT            1003
#define IDC_WORK            1004
#define IDC_REST            1005
#define IDC_OVERLAY         1006
#define IDC_OVERLAY_MODE    1007
#define IDC_CODE            1008
#define IDC_WARNING         1009
#define IDC_WARN_POS        1010
#define IDC_HK_PAUSE        1011
#define IDC_HK_SKIP         1012
#define IDC_AUTOSTART       1013
#define IDC_SAVE            1014
#define IDC_FONT_BROWSE     1015
#define IDC_HK_PAUSE_BROWSE 1016
#define IDC_HK_SKIP_BROWSE  1017

// 所有控件句柄
HWND g_hLang, g_hTheme, g_hFont, g_hWork, g_hRest;
HWND g_hOverlay, g_hOverlayMode, g_hCode, g_hWarning, g_hWarnPos;
HWND g_hHkPause, g_hHkSkip, g_hAutoStart, g_hSave;
HWND g_hFontBrowse = NULL;
HWND g_hHkPauseBrowse = NULL;
HWND g_hHkSkipBrowse   = NULL;
// 静态标签句柄
HWND g_hLabelLang, g_hLabelTheme, g_hLabelFont, g_hLabelWork, g_hLabelRest;
HWND g_hLabelOverlayMode, g_hLabelWarnPos, g_hLabelHkPause, g_hLabelHkSkip;
// 分组框句柄
HWND g_hGroupGeneral, g_hGroupTiming, g_hGroupOverlay, g_hGroupHotkey, g_hGroupOther;

int g_dpiX = 96, g_dpiY = 96;
int g_scrollY = 0;          // 当前垂直滚动偏移（像素）
int g_totalHeight = 0;      // 所有控件所需的总高度（像素）

int ScaleX(int x) { return MulDiv(x, g_dpiX, 96); }
int ScaleY(int y) { return MulDiv(y, g_dpiY, 96); }

// ---- 多语言字符串枚举 ----
enum StringID {
    STR_LANG,
    STR_THEME,
    STR_FONT,
    STR_WORK,
    STR_REST,
    STR_OVERLAY,
    STR_OVERLAY_MODE,
    STR_CODE,
    STR_WARNING,
    STR_WARN_POS,
    STR_HK_PAUSE,
    STR_HK_SKIP,
    STR_AUTOSTART,
    STR_SAVE,
    STR_FONT_BROWSE,
    STR_THEME_DARK,
    STR_THEME_LIGHT,
    STR_OVERLAY_FULLSCREEN,
    STR_OVERLAY_SECUREDESKTOP,
    STR_POS_BR,
    STR_POS_BL,
    STR_POS_TR,
    STR_POS_TL,
    STR_POS_CENTER,
    STR_POS_TOP_CENTER,
    STR_POS_BOTTOM_CENTER,
    STR_WINDOW_TITLE,
    STR_ERR_WORK_RANGE,
    STR_ERR_REST_RANGE,
    STR_SAVE_SUCCESS,
    STR_SAVE_FAIL,
    STR_MSG_TITLE_ERROR,
    STR_MSG_TITLE_SUCCESS,
    STR_MSG_TITLE_WARNING,
    STR_GROUP_GENERAL,
    STR_GROUP_TIMING,
    STR_GROUP_OVERLAY,
    STR_GROUP_HOTKEY,
    STR_GROUP_OTHER,
};

// 多语言字符串表
std::wstring GetString(int id) {
    // 中文（默认）
    static const std::map<int, std::wstring> zh = {
        {STR_LANG, L"语言:"},
        {STR_THEME, L"主题:"},
        {STR_FONT, L"字体:"},
        {STR_WORK, L"专注 (分钟):"},
        {STR_REST, L"休息 (秒):"},
        {STR_OVERLAY, L"强制提醒弹窗"},
        {STR_OVERLAY_MODE, L"弹窗模式:"},
        {STR_CODE, L"需输入验证码"},
        {STR_WARNING, L"休息提醒(提前10秒)"},
        {STR_WARN_POS, L"提醒位置:"},
        {STR_HK_PAUSE, L"暂停热键:"},
        {STR_HK_SKIP, L"跳过热键:"},
        {STR_AUTOSTART, L"开机自启动"},
        {STR_SAVE, L"保存设置"},
        {STR_FONT_BROWSE, L"选择..."},
        {STR_THEME_DARK, L"深色"},
        {STR_THEME_LIGHT, L"浅色"},
        {STR_OVERLAY_FULLSCREEN, L"全屏弹窗"},
        {STR_OVERLAY_SECUREDESKTOP, L"安全桌面"},
        {STR_POS_BR, L"右下角"},
        {STR_POS_BL, L"左下角"},
        {STR_POS_TR, L"右上角"},
        {STR_POS_TL, L"左上角"},
        {STR_POS_CENTER, L"中间"},
        {STR_POS_TOP_CENTER, L"中上方"},
        {STR_POS_BOTTOM_CENTER, L"中下方"},
        {STR_WINDOW_TITLE, L"设置 - 20-20-20"},
        {STR_ERR_WORK_RANGE, L"专注时间必须为 1 到 120 分钟之间的整数！"},
        {STR_ERR_REST_RANGE, L"休息时间必须为 5 到 300 秒之间的整数！"},
        {STR_SAVE_SUCCESS, L"设置已保存，请重启主程序生效。"},
        {STR_SAVE_FAIL, L"保存设置文件失败！"},
        {STR_MSG_TITLE_ERROR, L"输入错误"},
        {STR_MSG_TITLE_SUCCESS, L"成功"},
        {STR_MSG_TITLE_WARNING, L"错误"},
        {STR_GROUP_GENERAL, L"常规"},
        {STR_GROUP_TIMING, L"计时"},
        {STR_GROUP_OVERLAY, L"强制提醒"},
        {STR_GROUP_HOTKEY, L"热键"},
        {STR_GROUP_OTHER, L"其他"},
    };
    static const std::map<int, std::wstring> en = {
        {STR_LANG, L"Language:"},
        {STR_THEME, L"Theme:"},
        {STR_FONT, L"Font:"},
        {STR_WORK, L"Focus (minutes):"},
        {STR_REST, L"Rest (seconds):"},
        {STR_OVERLAY, L"Force Overlay"},
        {STR_OVERLAY_MODE, L"Overlay Mode:"},
        {STR_CODE, L"Require Verification Code"},
        {STR_WARNING, L"Rest Warning (10s before)"},
        {STR_WARN_POS, L"Warning Position:"},
        {STR_HK_PAUSE, L"Pause Hotkey:"},
        {STR_HK_SKIP, L"Skip Hotkey:"},
        {STR_AUTOSTART, L"Auto Start with Windows"},
        {STR_SAVE, L"Save Settings"},
        {STR_FONT_BROWSE, L"Choose..."},
        {STR_THEME_DARK, L"Dark"},
        {STR_THEME_LIGHT, L"Light"},
        {STR_OVERLAY_FULLSCREEN, L"Fullscreen Overlay"},
        {STR_OVERLAY_SECUREDESKTOP, L"Secure Desktop"},
        {STR_POS_BR, L"Bottom Right"},
        {STR_POS_BL, L"Bottom Left"},
        {STR_POS_TR, L"Top Right"},
        {STR_POS_TL, L"Top Left"},
        {STR_POS_CENTER, L"Center"},
        {STR_POS_TOP_CENTER, L"Top Center"},
        {STR_POS_BOTTOM_CENTER, L"Bottom Center"},
        {STR_WINDOW_TITLE, L"Settings - 20-20-20"},
        {STR_ERR_WORK_RANGE, L"Focus time must be an integer between 1 and 120 minutes!"},
        {STR_ERR_REST_RANGE, L"Rest time must be an integer between 5 and 300 seconds!"},
        {STR_SAVE_SUCCESS, L"Settings saved. Please restart main program to apply."},
        {STR_SAVE_FAIL, L"Failed to save settings file!"},
        {STR_MSG_TITLE_ERROR, L"Input Error"},
        {STR_MSG_TITLE_SUCCESS, L"Success"},
        {STR_MSG_TITLE_WARNING, L"Error"},
        {STR_GROUP_GENERAL, L"General"},
        {STR_GROUP_TIMING, L"Timing"},
        {STR_GROUP_OVERLAY, L"Force Overlay"},
        {STR_GROUP_HOTKEY, L"Hotkeys"},
        {STR_GROUP_OTHER, L"Other"},
    };
    static const std::map<int, std::wstring> ru = {
        {STR_LANG, L"Язык:"},
        {STR_THEME, L"Тема:"},
        {STR_FONT, L"Шрифт:"},
        {STR_WORK, L"Фокус (минуты):"},
        {STR_REST, L"Отдых (секунды):"},
        {STR_OVERLAY, L"Принудительное окно"},
        {STR_OVERLAY_MODE, L"Режим окна:"},
        {STR_CODE, L"Требовать код подтверждения"},
        {STR_WARNING, L"Предупреждение (за 10 сек)"},
        {STR_WARN_POS, L"Позиция предупреждения:"},
        {STR_HK_PAUSE, L"Горячая клавиша паузы:"},
        {STR_HK_SKIP, L"Горячая клавиша пропуска:"},
        {STR_AUTOSTART, L"Автозапуск с Windows"},
        {STR_SAVE, L"Сохранить"},
        {STR_FONT_BROWSE, L"Выбрать..."},
        {STR_THEME_DARK, L"Тёмная"},
        {STR_THEME_LIGHT, L"Светлая"},
        {STR_OVERLAY_FULLSCREEN, L"Полноэкранное окно"},
        {STR_OVERLAY_SECUREDESKTOP, L"Безопасный рабочий стол"},
        {STR_POS_BR, L"Снизу справа"},
        {STR_POS_BL, L"Снизу слева"},
        {STR_POS_TR, L"Сверху справа"},
        {STR_POS_TL, L"Сверху слева"},
        {STR_POS_CENTER, L"Центр"},
        {STR_POS_TOP_CENTER, L"Центр сверху"},
        {STR_POS_BOTTOM_CENTER, L"Центр снизу"},
        {STR_WINDOW_TITLE, L"Настройки - 20-20-20"},
        {STR_ERR_WORK_RANGE, L"Время фокуса должно быть целым числом от 1 до 120 минут!"},
        {STR_ERR_REST_RANGE, L"Время отдыха должно быть целым числом от 5 до 300 секунд!"},
        {STR_SAVE_SUCCESS, L"Настройки сохранены. Перезапустите основную программу для применения."},
        {STR_SAVE_FAIL, L"Не удалось сохранить файл настроек!"},
        {STR_MSG_TITLE_ERROR, L"Ошибка ввода"},
        {STR_MSG_TITLE_SUCCESS, L"Успех"},
        {STR_MSG_TITLE_WARNING, L"Ошибка"},
        {STR_GROUP_GENERAL, L"Общие"},
        {STR_GROUP_TIMING, L"Время"},
        {STR_GROUP_OVERLAY, L"Принудительное окно"},
        {STR_GROUP_HOTKEY, L"Горячие клавиши"},
        {STR_GROUP_OTHER, L"Прочее"},
    };

    const std::map<int, std::wstring>* langMap = &zh;
    if (g_settings.lang == L"en")
        langMap = &en;
    else if (g_settings.lang == L"ru")
        langMap = &ru;

    auto it = langMap->find(id);
    if (it != langMap->end())
        return it->second;
    return L"";
}

// ---- JSON 读写 ----
std::wstring EscapeJson(const std::wstring& s) {
    std::wstring res;
    for (wchar_t c : s) {
        if (c == L'\"' || c == L'\\') res += L'\\';
        res += c;
    }
    return res;
}

bool LoadSettings() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    std::wstring jsonPath = std::wstring(exePath) + L"\\settings.json";

    std::ifstream file(jsonPath.c_str());
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto findValue = [&](const std::string& key) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = content.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
        if (pos >= content.size()) return "";
        if (content[pos] == '\"') {
            pos++;
            size_t end = content.find('\"', pos);
            if (end == std::string::npos) return "";
            return content.substr(pos, end - pos);
        } else if (content[pos] == 't' || content[pos] == 'f') {
            size_t end = content.find_first_of(",}\n", pos);
            if (end == std::string::npos) return "";
            return content.substr(pos, end - pos);
        } else {
            size_t end = content.find_first_of(",}\n", pos);
            if (end == std::string::npos) return "";
            return content.substr(pos, end - pos);
        }
    };

    auto findBool = [&](const std::string& key) -> bool {
        std::string val = findValue(key);
        if (val.empty()) return false;
        return val == "true";
    };

    auto findInt = [&](const std::string& key) -> int {
        std::string val = findValue(key);
        if (val.empty()) return 0;
        return std::stoi(val);
    };

    g_settings.lang = Utf8ToWide(findValue("lang"));
    g_settings.work_duration = findInt("work_duration");
    g_settings.rest_duration = findInt("rest_duration");
    g_settings.use_overlay = findBool("use_overlay");
    g_settings.require_code = findBool("require_code");
    g_settings.use_warning = findBool("use_warning");
    g_settings.warn_position = Utf8ToWide(findValue("warn_position"));
    g_settings.hotkey_pause = Utf8ToWide(findValue("hotkey_pause"));
    g_settings.hotkey_skip = Utf8ToWide(findValue("hotkey_skip"));
    g_settings.theme = Utf8ToWide(findValue("theme"));
    g_settings.font_name = Utf8ToWide(findValue("font_name"));
    g_settings.auto_start = findBool("auto_start");
    g_settings.overlay_mode = Utf8ToWide(findValue("overlay_mode"));

    return true;
}

bool SaveSettings() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    std::wstring jsonPath = std::wstring(exePath) + L"\\settings.json";

    std::wstringstream ss;
    ss << L"{\n";
    ss << L"  \"lang\": \"" << EscapeJson(g_settings.lang) << L"\",\n";
    ss << L"  \"work_duration\": " << g_settings.work_duration << L",\n";
    ss << L"  \"rest_duration\": " << g_settings.rest_duration << L",\n";
    ss << L"  \"use_overlay\": " << (g_settings.use_overlay ? L"true" : L"false") << L",\n";
    ss << L"  \"require_code\": " << (g_settings.require_code ? L"true" : L"false") << L",\n";
    ss << L"  \"use_warning\": " << (g_settings.use_warning ? L"true" : L"false") << L",\n";
    ss << L"  \"warn_position\": \"" << EscapeJson(g_settings.warn_position) << L"\",\n";
    ss << L"  \"hotkey_pause\": \"" << EscapeJson(g_settings.hotkey_pause) << L"\",\n";
    ss << L"  \"hotkey_skip\": \"" << EscapeJson(g_settings.hotkey_skip) << L"\",\n";
    ss << L"  \"theme\": \"" << EscapeJson(g_settings.theme) << L"\",\n";
    ss << L"  \"font_name\": \"" << EscapeJson(g_settings.font_name) << L"\",\n";
    ss << L"  \"total_cycles\": 0,\n";
    ss << L"  \"auto_start\": " << (g_settings.auto_start ? L"true" : L"false") << L",\n";
    ss << L"  \"overlay_mode\": \"" << EscapeJson(g_settings.overlay_mode) << L"\"\n";
    ss << L"}";

    std::ofstream file(jsonPath.c_str(), std::ios::binary);
    if (!file.is_open()) return false;

    std::string utf8;
    int len = WideCharToMultiByte(CP_UTF8, 0, ss.str().c_str(), -1, NULL, 0, NULL, NULL);
    if (len > 0) {
        std::vector<char> buffer(len);
        WideCharToMultiByte(CP_UTF8, 0, ss.str().c_str(), -1, buffer.data(), len, NULL, NULL);
        utf8 = buffer.data();
    }
    file.write(utf8.c_str(), utf8.size());
    file.close();
    return true;
}

void SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    if (enable) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);          // 当前 settings.exe 完整路径
        PathRemoveFileSpecW(exePath);                         // 去掉文件名，只剩目录
        PathAppendW(exePath, L"SaveYourPeepersCPP.exe");      // 拼接目标主程序名

        RegSetValueExW(hKey, L"20-20-20", 0, REG_SZ,
                       (BYTE*)exePath,
                       (wcslen(exePath) + 1) * sizeof(wchar_t));
    } else {
        RegDeleteValueW(hKey, L"20-20-20");
    }
    RegCloseKey(hKey);
}

// ---- 刷新界面所有文本（多语言） ----
void RefreshUI(HWND hWnd) {
    // 窗口标题
    SetWindowTextW(hWnd, GetString(STR_WINDOW_TITLE).c_str());

    // 分组框标题
    SetWindowTextW(g_hGroupGeneral, GetString(STR_GROUP_GENERAL).c_str());
    SetWindowTextW(g_hGroupTiming,  GetString(STR_GROUP_TIMING).c_str());
    SetWindowTextW(g_hGroupOverlay, GetString(STR_GROUP_OVERLAY).c_str());
    SetWindowTextW(g_hGroupHotkey,  GetString(STR_GROUP_HOTKEY).c_str());
    SetWindowTextW(g_hGroupOther,   GetString(STR_GROUP_OTHER).c_str());

    // 静态标签
    SetWindowTextW(g_hLabelLang, GetString(STR_LANG).c_str());
    SetWindowTextW(g_hLabelTheme, GetString(STR_THEME).c_str());
    SetWindowTextW(g_hLabelFont, GetString(STR_FONT).c_str());
    SetWindowTextW(g_hLabelWork, GetString(STR_WORK).c_str());
    SetWindowTextW(g_hLabelRest, GetString(STR_REST).c_str());
    SetWindowTextW(g_hLabelOverlayMode, GetString(STR_OVERLAY_MODE).c_str());
    SetWindowTextW(g_hLabelWarnPos, GetString(STR_WARN_POS).c_str());
    SetWindowTextW(g_hLabelHkPause, GetString(STR_HK_PAUSE).c_str());
    SetWindowTextW(g_hLabelHkSkip, GetString(STR_HK_SKIP).c_str());

    // 复选框
    SetWindowTextW(g_hOverlay, GetString(STR_OVERLAY).c_str());
    SetWindowTextW(g_hCode, GetString(STR_CODE).c_str());
    SetWindowTextW(g_hWarning, GetString(STR_WARNING).c_str());
    SetWindowTextW(g_hAutoStart, GetString(STR_AUTOSTART).c_str());

    // 按钮
    SetWindowTextW(g_hSave, GetString(STR_SAVE).c_str());
    SetWindowTextW(g_hFontBrowse, GetString(STR_FONT_BROWSE).c_str());

    // 更新热键选择按钮文本
    SetWindowTextW(g_hHkPauseBrowse, GetString(STR_FONT_BROWSE).c_str());
    SetWindowTextW(g_hHkSkipBrowse, GetString(STR_FONT_BROWSE).c_str());

    // ---- 刷新下拉框选项（重建） ----
    // 主题下拉框
    SendMessage(g_hTheme, CB_RESETCONTENT, 0, 0);
    SendMessage(g_hTheme, CB_ADDSTRING, 0, (LPARAM)GetString(STR_THEME_DARK).c_str());
    SendMessage(g_hTheme, CB_ADDSTRING, 0, (LPARAM)GetString(STR_THEME_LIGHT).c_str());
    int themeIdx = (g_settings.theme == L"Light") ? 1 : 0;
    SendMessage(g_hTheme, CB_SETCURSEL, themeIdx, 0);

    // 弹窗模式下拉框
    SendMessage(g_hOverlayMode, CB_RESETCONTENT, 0, 0);
    SendMessage(g_hOverlayMode, CB_ADDSTRING, 0, (LPARAM)GetString(STR_OVERLAY_FULLSCREEN).c_str());
    SendMessage(g_hOverlayMode, CB_ADDSTRING, 0, (LPARAM)GetString(STR_OVERLAY_SECUREDESKTOP).c_str());
    int modeIdx = (g_settings.overlay_mode == L"securedesktop") ? 1 : 0;
    SendMessage(g_hOverlayMode, CB_SETCURSEL, modeIdx, 0);

    // 提醒位置下拉框
    SendMessage(g_hWarnPos, CB_RESETCONTENT, 0, 0);
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_BR).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_BL).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_TR).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_TL).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_CENTER).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_TOP_CENTER).c_str());
    SendMessage(g_hWarnPos, CB_ADDSTRING, 0, (LPARAM)GetString(STR_POS_BOTTOM_CENTER).c_str());
    int posIdx = 0;
    if (g_settings.warn_position == L"pos_bl") posIdx = 1;
    else if (g_settings.warn_position == L"pos_tr") posIdx = 2;
    else if (g_settings.warn_position == L"pos_tl") posIdx = 3;
    else if (g_settings.warn_position == L"pos_center") posIdx = 4;
    else if (g_settings.warn_position == L"pos_top_center") posIdx = 5;
    else if (g_settings.warn_position == L"pos_bottom_center") posIdx = 6;
    SendMessage(g_hWarnPos, CB_SETCURSEL, posIdx, 0);

    // 语言下拉框
    int langIdx = 2; // 默认中文
    if (g_settings.lang == L"ru") langIdx = 0;
    else if (g_settings.lang == L"en") langIdx = 1;
    SendMessage(g_hLang, CB_SETCURSEL, langIdx, 0);
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HDC hdc = GetDC(hWnd);
            g_dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            g_dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hWnd, hdc);

            LoadSettings();
            g_settings_loaded = true;

            // 应用标题栏主题（深色/浅色）
            ApplyTitleBarTheme(hWnd);

            // 创建画刷
            g_hBrushDark  = CreateSolidBrush(COLOR_DARK_BG);
            g_hBrushLight = CreateSolidBrush(COLOR_LIGHT_BG);
            g_hBrushDarkCtrl  = CreateSolidBrush(COLOR_DARK_CTRL);
            g_hBrushLightCtrl = CreateSolidBrush(COLOR_LIGHT_CTRL);

            int fontHeight = -ScaleY(14);
            g_hUIFont = CreateFontW(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

            auto CreateLabel = [&](int x, int y, int width, int height, const wchar_t* text) -> HWND {
                HWND hCtrl = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          ScaleX(x), ScaleY(y), ScaleX(width), ScaleY(height),
                                          hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hCtrl, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                return hCtrl;
            };

            auto CreateCombo = [&](int x, int y, int width, int height) -> HWND {
                HWND hCtrl = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                          ScaleX(x), ScaleY(y), ScaleX(width), ScaleY(height),
                                          hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hCtrl, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                return hCtrl;
            };

            auto CreateEdit = [&](int x, int y, int width, int height, bool esNumber = false) -> HWND {
                DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
                if (esNumber) style |= ES_NUMBER;
                HWND hCtrl = CreateWindowW(L"EDIT", NULL, style,
                                          ScaleX(x), ScaleY(y), ScaleX(width), ScaleY(height),
                                          hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hCtrl, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                return hCtrl;
            };

            auto CreateCheck = [&](int x, int y, int width, int height, const wchar_t* text) -> HWND {
                HWND hCtrl = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                          ScaleX(x), ScaleY(y), ScaleX(width), ScaleY(height),
                                          hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hCtrl, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                return hCtrl;
            };

            auto CreateGroupBox = [&](int x, int y, int width, int height, const wchar_t* text) -> HWND {
                HWND hGroup = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                            ScaleX(x), ScaleY(y), ScaleX(width), ScaleY(height),
                                            hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hGroup, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                return hGroup;
            };

            // 创建分组框
            g_hGroupGeneral = CreateGroupBox(20, 20, 550, 135, L"");
            g_hGroupTiming  = CreateGroupBox(20, 160, 550, 100, L"");
            g_hGroupOverlay = CreateGroupBox(20, 270, 550, 210, L"");
            g_hGroupHotkey  = CreateGroupBox(20, 480, 550, 100, L"");
            g_hGroupOther   = CreateGroupBox(20, 590, 550, 60, L"");

            int labelW = 140, ctrlW = 220, ctrlH = 26, gap = 10;
            int groupInnerX = 40;
            int y = 45;

            // ---- 常规 ----
            g_hLabelLang = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hLang = CreateCombo(groupInnerX + labelW + gap, y, ctrlW, 100);
            SendMessage(g_hLang, CB_ADDSTRING, 0, (LPARAM)L"Русский");
            SendMessage(g_hLang, CB_ADDSTRING, 0, (LPARAM)L"English");
            SendMessage(g_hLang, CB_ADDSTRING, 0, (LPARAM)L"中文");
            y += ctrlH + 12;

            g_hLabelTheme = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hTheme = CreateCombo(groupInnerX + labelW + gap, y, ctrlW, 100);
            y += ctrlH + 12;

            g_hLabelFont = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hFont = CreateEdit(groupInnerX + labelW + gap, y, ctrlW, ctrlH);
            SetWindowTextW(g_hFont, g_settings.font_name.c_str());
            // 字体浏览按钮：自绘
            g_hFontBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                          ScaleX(groupInnerX + labelW + gap + ctrlW + 10), ScaleY(y),
                                          ScaleX(70), ScaleY(ctrlH),
                                          hWnd, (HMENU)IDC_FONT_BROWSE, GetModuleHandle(NULL), NULL);
            SendMessage(g_hFontBrowse, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

            // ---- 计时 ----
            y = 185;
            g_hLabelWork = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hWork = CreateEdit(groupInnerX + labelW + gap, y, ctrlW, ctrlH, true);
            wchar_t buf[16];
            swprintf(buf, 16, L"%d", g_settings.work_duration / 60);
            SetWindowTextW(g_hWork, buf);
            y += ctrlH + 12;

            g_hLabelRest = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hRest = CreateEdit(groupInnerX + labelW + gap, y, ctrlW, ctrlH, true);
            swprintf(buf, 16, L"%d", g_settings.rest_duration);
            SetWindowTextW(g_hRest, buf);

            // ---- 强制提醒 ----
            y = 295;
            g_hOverlay = CreateCheck(groupInnerX, y, 260, ctrlH, L"");
            SendMessage(g_hOverlay, BM_SETCHECK, g_settings.use_overlay ? BST_CHECKED : BST_UNCHECKED, 0);
            y += ctrlH + 12;

            g_hLabelOverlayMode = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hOverlayMode = CreateCombo(groupInnerX + labelW + gap, y, ctrlW, 100);
            y += ctrlH + 12;

            g_hCode = CreateCheck(groupInnerX, y, 260, ctrlH, L"");
            SendMessage(g_hCode, BM_SETCHECK, g_settings.require_code ? BST_CHECKED : BST_UNCHECKED, 0);
            y += ctrlH + 12;

            g_hWarning = CreateCheck(groupInnerX, y, 260, ctrlH, L"");
            SendMessage(g_hWarning, BM_SETCHECK, g_settings.use_warning ? BST_CHECKED : BST_UNCHECKED, 0);
            y += ctrlH + 12;

            g_hLabelWarnPos = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hWarnPos = CreateCombo(groupInnerX + labelW + gap, y, ctrlW, 100);

            // ---- 热键 ----
            y = 505;
            g_hLabelHkPause = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hHkPause = CreateEdit(groupInnerX + labelW + gap, y, ctrlW, ctrlH);
            SetWindowTextW(g_hHkPause, g_settings.hotkey_pause.c_str());
            // 创建暂停热键的"选择..."按钮
            g_hHkPauseBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                  ScaleX(groupInnerX + labelW + gap + ctrlW + 10), ScaleY(y),
                                  ScaleX(70), ScaleY(ctrlH),
                                  hWnd, (HMENU)IDC_HK_PAUSE_BROWSE, GetModuleHandle(NULL), NULL);
            SendMessage(g_hHkPauseBrowse, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
            SetWindowTextW(g_hHkPauseBrowse, GetString(STR_FONT_BROWSE).c_str());
            y += ctrlH + 12;

            g_hLabelHkSkip = CreateLabel(groupInnerX, y, labelW, ctrlH, L"");
            g_hHkSkip = CreateEdit(groupInnerX + labelW + gap, y, ctrlW, ctrlH);
            SetWindowTextW(g_hHkSkip, g_settings.hotkey_skip.c_str());
            // 创建跳过热键的"选择..."按钮
            g_hHkSkipBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                 ScaleX(groupInnerX + labelW + gap + ctrlW + 10), ScaleY(y),
                                 ScaleX(70), ScaleY(ctrlH),
                                 hWnd, (HMENU)IDC_HK_SKIP_BROWSE, GetModuleHandle(NULL), NULL);
            SendMessage(g_hHkSkipBrowse, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
            SetWindowTextW(g_hHkSkipBrowse, GetString(STR_FONT_BROWSE).c_str());

            // ---- 其他 ----
            y = 615;
            g_hAutoStart = CreateCheck(groupInnerX, y, 260, ctrlH, L"");
            SendMessage(g_hAutoStart, BM_SETCHECK, g_settings.auto_start ? BST_CHECKED : BST_UNCHECKED, 0);

            // ---- 保存按钮（自绘） ----
            g_hSave = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                    ScaleX(20), ScaleY(660), ScaleX(120), ScaleY(32),
                                    hWnd, (HMENU)IDC_SAVE, GetModuleHandle(NULL), NULL);
            SendMessage(g_hSave, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

            // 设置窗口大小
            SetWindowPos(hWnd, NULL, 0, 0, ScaleX(600), ScaleY(550), SWP_NOMOVE | SWP_NOZORDER);

            // 计算控件总高度（取最底部控件的底部坐标 + 边距）
            // 当前最底部是保存按钮，其 y=660，高度 32，所以底部为 660+32=692，再加一些边距
            g_totalHeight = ScaleY(700) + 50;   // 或者更精确：遍历子控件取最大底部
            // 初始化滚动信息
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_RANGE | SIF_PAGE;
            si.nMin = 0;
            si.nMax = g_totalHeight - 1;
            si.nPage = ScaleY(700);   // 客户区高度（假设窗口高度不变）
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

            // 刷新界面为当前语言
            RefreshUI(hWnd);
            break;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_FONT_BROWSE) {
                LOGFONTW lf = {0};
                GetWindowTextW(g_hFont, lf.lfFaceName, LF_FACESIZE);
                lf.lfHeight = -ScaleY(14);

                CHOOSEFONTW cf = {0};
                cf.lStructSize = sizeof(cf);
                cf.hwndOwner = hWnd;
                cf.lpLogFont = &lf;
                cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_ENABLEHOOK;
                cf.lpfnHook = FontDialogHook;

                if (ChooseFontW(&cf)) {
                    SetWindowTextW(g_hFont, lf.lfFaceName);
                    g_settings.font_name = lf.lfFaceName;
                    if (g_hUIFont) DeleteObject(g_hUIFont);
                    g_hUIFont = CreateFontW(
                        -ScaleY(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, lf.lfFaceName);
                    EnumChildWindows(hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
                        SendMessage(hChild, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
                        return TRUE;
                    }, 0);
                }
                return 0;
            }

            if (LOWORD(wParam) == IDC_HK_PAUSE_BROWSE) {
                ShowHotkeyCaptureDialog(hWnd, g_hHkPause);
                return 0;
            }
            if (LOWORD(wParam) == IDC_HK_SKIP_BROWSE) {
                ShowHotkeyCaptureDialog(hWnd, g_hHkSkip);
                return 0;
            }

            // 语言下拉框切换 -> 即时刷新界面
            if (LOWORD(wParam) == IDC_LANG && HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = SendMessage(g_hLang, CB_GETCURSEL, 0, 0);
                if (sel == 0) g_settings.lang = L"ru";
                else if (sel == 1) g_settings.lang = L"en";
                else g_settings.lang = L"zh";
                RefreshUI(hWnd);
                return 0;
            }

            // 主题下拉框切换 -> 实时预览标题栏
            if (LOWORD(wParam) == IDC_THEME && HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = SendMessage(g_hTheme, CB_GETCURSEL, 0, 0);
                g_settings.theme = (sel == 1) ? L"Light" : L"Dark";
                ApplyTitleBarTheme(hWnd);
                InvalidateRect(hWnd, NULL, TRUE);
                return 0;
            }

            if (LOWORD(wParam) == IDC_SAVE) {
                int langSel = SendMessage(g_hLang, CB_GETCURSEL, 0, 0);
                if (langSel == 0) g_settings.lang = L"ru";
                else if (langSel == 1) g_settings.lang = L"en";
                else g_settings.lang = L"zh";

                int themeSel = SendMessage(g_hTheme, CB_GETCURSEL, 0, 0);
                g_settings.theme = (themeSel == 1) ? L"Light" : L"Dark";

                wchar_t fontBuf[256];
                GetWindowTextW(g_hFont, fontBuf, 256);
                g_settings.font_name = fontBuf;

                wchar_t workBuf[16];
                GetWindowTextW(g_hWork, workBuf, 16);
                int workMin = _wtoi(workBuf);
                if (workMin < 1 || workMin > 120) {
                    MessageBoxW(hWnd, GetString(STR_ERR_WORK_RANGE).c_str(),
                                GetString(STR_MSG_TITLE_ERROR).c_str(), MB_OK | MB_ICONWARNING);
                    SetFocus(g_hWork);
                    return 0;
                }
                g_settings.work_duration = workMin * 60;

                wchar_t restBuf[16];
                GetWindowTextW(g_hRest, restBuf, 16);
                int restSec = _wtoi(restBuf);
                if (restSec < 5 || restSec > 300) {
                    MessageBoxW(hWnd, GetString(STR_ERR_REST_RANGE).c_str(),
                                GetString(STR_MSG_TITLE_ERROR).c_str(), MB_OK | MB_ICONWARNING);
                    SetFocus(g_hRest);
                    return 0;
                }
                g_settings.rest_duration = restSec;

                g_settings.use_overlay = (SendMessage(g_hOverlay, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_settings.require_code = (SendMessage(g_hCode, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_settings.use_warning = (SendMessage(g_hWarning, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_settings.auto_start = (SendMessage(g_hAutoStart, BM_GETCHECK, 0, 0) == BST_CHECKED);

                int modeSel = SendMessage(g_hOverlayMode, CB_GETCURSEL, 0, 0);
                g_settings.overlay_mode = (modeSel == 1) ? L"securedesktop" : L"fullscreen";

                int posSel = SendMessage(g_hWarnPos, CB_GETCURSEL, 0, 0);
                switch (posSel) {
                    case 0: g_settings.warn_position = L"pos_br"; break;
                    case 1: g_settings.warn_position = L"pos_bl"; break;
                    case 2: g_settings.warn_position = L"pos_tr"; break;
                    case 3: g_settings.warn_position = L"pos_tl"; break;
                    case 4: g_settings.warn_position = L"pos_center"; break;
                    case 5: g_settings.warn_position = L"pos_top_center"; break;
                    case 6: g_settings.warn_position = L"pos_bottom_center"; break;
                }

                wchar_t hkBuf[64];
                GetWindowTextW(g_hHkPause, hkBuf, 64);
                g_settings.hotkey_pause = hkBuf;
                GetWindowTextW(g_hHkSkip, hkBuf, 64);
                g_settings.hotkey_skip = hkBuf;

                auto IsValidHotkey = [](const std::wstring& hk) -> bool {
    if (hk.empty()) return false;

    // 1. 去除首尾空白字符（防呆）
    std::wstring trimmed = hk;
    size_t start = trimmed.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return false;
    size_t end = trimmed.find_last_not_of(L" \t\r\n");
    trimmed = trimmed.substr(start, end - start + 1);

    // 2. 统一转为小写便于比较
    std::wstring lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    // 3. 按 '+' 分割，并杜绝连续 '+' 或首尾 '+' 等异常
    std::vector<std::wstring> parts;
    std::wstring part;
    for (wchar_t ch : lower) {
        if (ch == L'+') {
            if (part.empty()) return false; // 出现 "++" 或开头结尾的 '+'
            parts.push_back(part);
            part.clear();
        } else {
            part.push_back(ch);
        }
    }
    if (part.empty()) return false; // 以 '+' 结尾
    parts.push_back(part);

    // 必须至少有一个修饰键和一个键名
    if (parts.size() < 2) return false;

    // 4. 检查所有前面的部分是否都是合法的修饰键（白名单）
    const std::set<std::wstring> validMods = { L"ctrl", L"shift", L"alt" };
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (validMods.find(parts[i]) == validMods.end()) {
            return false; // 例如 "1ctrl" 或 "abc" 都会在这里被拦截
        }
    }

    // 5. 校验最后一部分（键名）
    std::wstring key = parts.back();

    // 5.1 单字符键：允许任意字母、数字或常见符号（如 '-', '=', '[' 等）
    if (key.size() == 1) {
        return true;
    }

    // 5.2 功能键 F1 ~ F24
    if (key[0] == L'f' && key.size() > 1) {
        bool allDigits = true;
        for (size_t i = 1; i < key.size(); ++i) {
            if (!iswdigit(key[i])) { allDigits = false; break; }
        }
        if (allDigits) {
            int num = std::stoi(key.substr(1));
            if (num >= 1 && num <= 24) return true;
        }
    }

    // 5.3 常用特殊键白名单（与 GetKeyNameText 返回值保持一致）
    const std::set<std::wstring> specialKeys = {
        // 导航与编辑
        L"home", L"end", L"pgup", L"pgdn", L"insert", L"delete",
        // 方向键
        L"up", L"down", L"left", L"right",
        // 控制与符号
        L"space", L"tab", L"enter", L"escape", L"esc",
        L"backspace", L"back",
        // 系统与状态
        L"printscreen", L"pause", L"break",
        L"numlock", L"capslock", L"scrolllock",
        // 数字键盘
        L"add", L"subtract", L"multiply", L"divide", L"decimal",
        // Windows 键（若用户捕获得到，予以放行）
        L"apps", L"lwin", L"rwin", L"win"
    };
    if (specialKeys.find(key) != specialKeys.end()) {
        return true;
    }

    // 5.4 兼容 OEM 键（如 OemMinus, OemPeriod）和 Numpad 键（如 Num 1）
    if (key.find(L"oem") != std::wstring::npos ||
        key.find(L"num") != std::wstring::npos) {
        return true;
    }

    // 以上均不匹配，视为无效
    return false;
};

                if (!IsValidHotkey(g_settings.hotkey_pause) || !IsValidHotkey(g_settings.hotkey_skip)) {
                    MessageBoxW(hWnd, L"热键格式无效，请使用“选择”按钮重新选择。",
                                L"错误", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                if (g_settings.hotkey_pause == g_settings.hotkey_skip) {
                    MessageBoxW(hWnd, L"暂停热键和跳过热键不能相同。",
                                L"错误", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                SetAutoStart(g_settings.auto_start);

                if (SaveSettings()) {
                    MessageBoxW(hWnd, GetString(STR_SAVE_SUCCESS).c_str(),
                                GetString(STR_MSG_TITLE_SUCCESS).c_str(), MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxW(hWnd, GetString(STR_SAVE_FAIL).c_str(),
                                GetString(STR_MSG_TITLE_WARNING).c_str(), MB_OK | MB_ICONERROR);
                }

                ApplyTitleBarTheme(hWnd);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }

        case WM_SIZE: {
            // 获取当前客户区高度
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int clientHeight = rcClient.bottom - rcClient.top;
            // 更新滚动页面大小
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_PAGE;
            si.nPage = clientHeight;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            break;
        }

        case WM_VSCROLL: {
            int newScrollY = g_scrollY;
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);

            switch (LOWORD(wParam)) {
                case SB_LINEUP:      newScrollY -= 20; break;
                case SB_LINEDOWN:    newScrollY += 20; break;
                case SB_PAGEUP:      newScrollY -= si.nPage; break;
                case SB_PAGEDOWN:    newScrollY += si.nPage; break;
                case SB_THUMBTRACK:  newScrollY = si.nTrackPos; break;
                default: return 0;
            }
            // 限制范围
            newScrollY = std::max(0, std::min(newScrollY, g_totalHeight - (int)si.nPage));
            if (newScrollY != g_scrollY) {
                int delta = g_scrollY - newScrollY;   // 移动偏移量
                g_scrollY = newScrollY;

                // 移动所有子窗口
                EnumChildWindows(hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
                    int deltaY = (int)lParam;
                    RECT rc;
                    GetWindowRect(hChild, &rc);
                    // 转换为相对于父窗口客户区的坐标
                    POINT pt = { rc.left, rc.top };
                    ScreenToClient(GetParent(hChild), &pt);
                    SetWindowPos(hChild, NULL, pt.x, pt.y + deltaY, 0, 0,
                                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    return TRUE;
                }, delta);

                // 更新滚动条位置
                si.fMask = SIF_POS;
                si.nPos = g_scrollY;
                SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

                // 强制重绘（可选）
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            int lines = zDelta / WHEEL_DELTA;          // 通常为 ±1
            int scrollAmount = -lines * 30;            // 每滚一格移动 30 像素（方向取反）

            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);

            int newScrollY = g_scrollY + scrollAmount;
            newScrollY = std::max(0, std::min(newScrollY, g_totalHeight - (int)si.nPage));

            if (newScrollY != g_scrollY) {
                int delta = g_scrollY - newScrollY;
                g_scrollY = newScrollY;

                // 移动所有子窗口
                EnumChildWindows(hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
                    int deltaY = (int)lParam;
                    RECT rc;
                    GetWindowRect(hChild, &rc);
                    POINT pt = { rc.left, rc.top };
                    ScreenToClient(GetParent(hChild), &pt);
                    SetWindowPos(hChild, NULL, pt.x, pt.y + deltaY, 0, 0,
                                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    return TRUE;
                }, delta);

                // 更新滚动条位置
                si.fMask = SIF_POS;
                si.nPos = g_scrollY;
                SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            if (pDIS->CtlType == ODT_BUTTON) {
                HDC hdc = pDIS->hDC;
                RECT rc = pDIS->rcItem;
                bool isDark = (g_settings.theme == L"Dark");
                bool isPressed = (pDIS->itemState & ODS_SELECTED);
                bool isHover = (pDIS->itemState & ODS_HOTLIGHT);

                COLORREF bgColor;
                if (pDIS->CtlID == IDC_SAVE) {
                    // 不填充背景，只画边框
                    HPEN hPen = CreatePen(PS_SOLID, 1, isDark ? RGB(120,120,120) : RGB(160,160,160));
                    SelectObject(hdc, hPen);
                    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
                    DeleteObject(hPen);
                    // 获取按钮文字
                    wchar_t text[256] = {0};
                    GetWindowTextW(pDIS->hwndItem, text, 256);
                    // 文字颜色根据主题
                    SetTextColor(hdc, isDark ? RGB(220,220,220) : RGB(50,50,50));
                    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    return TRUE;
                } else {
                    bgColor = isDark ? RGB(70,70,70) : RGB(220,220,220);
                    if (isPressed) bgColor = isDark ? RGB(50,50,50) : RGB(190,190,190);
                    else if (isHover) bgColor = isDark ? RGB(85,85,85) : RGB(210,210,210);
                }

                HBRUSH hBrush = CreateSolidBrush(bgColor);
                HPEN hPen = CreatePen(PS_SOLID, 1, isDark ? COLOR_DARK_BORDER : COLOR_LIGHT_BORDER);
                HGDIOBJ oldBrush = SelectObject(hdc, hBrush);
                HGDIOBJ oldPen = SelectObject(hdc, hPen);

                RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(hBrush);
                DeleteObject(hPen);

                wchar_t text[256] = {0};
                GetWindowTextW(pDIS->hwndItem, text, 256);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, isDark ? RGB(255,255,255) : RGB(0,0,0));
                DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                return TRUE;
            }
            break;
        }

        case WM_DESTROY:
            if (g_hUIFont) DeleteObject(g_hUIFont);
            if (g_hBrushDark)  DeleteObject(g_hBrushDark);
            if (g_hBrushLight) DeleteObject(g_hBrushLight);
            if (g_hBrushDarkCtrl)  DeleteObject(g_hBrushDarkCtrl);
            if (g_hBrushLightCtrl) DeleteObject(g_hBrushLightCtrl);
            PostQuitMessage(0);
            break;

        case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hBrush = (g_settings.theme == L"Dark") ? g_hBrushDark : g_hBrushLight;
            FillRect(hdc, &rc, hBrush);
            return TRUE;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
        {
            HDC hdc = (HDC)wParam;
            bool isDark = (g_settings.theme == L"Dark");
            SetTextColor(hdc, isDark ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT);
            SetBkColor(hdc, isDark ? COLOR_DARK_BG : COLOR_LIGHT_BG);
            return (LRESULT)(isDark ? g_hBrushDark : g_hBrushLight);
        }

        case WM_CTLCOLORBTN:
        {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            bool isDark = (g_settings.theme == L"Dark");
            // 分组框特殊处理：透明背景
            if (GetWindowLongPtr(hCtrl, GWL_STYLE) & BS_GROUPBOX) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, isDark ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            // 复选框/普通按钮：使用窗口背景色
            SetBkColor(hdc, isDark ? COLOR_DARK_BG : COLOR_LIGHT_BG);
            SetTextColor(hdc, isDark ? COLOR_DARK_TEXT : COLOR_LIGHT_TEXT);
            return (LRESULT)(isDark ? g_hBrushDark : g_hBrushLight);
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    // 启用高DPI (Per-Monitor V2)
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetDPIAwarenessContextFunc)(HANDLE);
        SetDPIAwarenessContextFunc pSetDPIAwarenessContext =
            (SetDPIAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDPIAwarenessContext");
        if (pSetDPIAwarenessContext) {
            pSetDPIAwarenessContext((HANDLE)(-4));
        } else {
            SetProcessDPIAware();
        }
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {0};
wc.cbSize = sizeof(WNDCLASSEXW);
wc.style = CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc = WndProc;
wc.hInstance = hInstance;
wc.hCursor = LoadCursor(NULL, IDC_ARROW);
// 加载资源中的图标（资源名称为 "MAINICON"，与 .rc 文件一致）
wc.hIcon = LoadIconW(hInstance, L"MAINICON");
// 小图标通常与大图标相同
wc.hIconSm = wc.hIcon;
// 若加载失败，可回退到默认图标（可选）
if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, L"IDI_APPLICATION");
wc.hbrBackground = NULL;
wc.lpszClassName = L"SettingsWindowClass";
if (!RegisterClassExW(&wc)) return 0;

    HWND hWnd = CreateWindowExW(0, L"SettingsWindowClass", L"",
                                WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME | WS_VSCROLL,
                                CW_USEDEFAULT, CW_USEDEFAULT, 720, 720,
                                NULL, NULL, hInstance, NULL);
    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}