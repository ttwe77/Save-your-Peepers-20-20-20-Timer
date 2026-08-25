// SaveYourPeepersCPP.cpp
// windres resource.rc -O coff -o resource.res
// g++ -std=c++17 -o SaveYourPeepersCPP.exe SaveYourPeepersCPP.cpp resource.res -luser32 -lgdi32 -lcomctl32 -lshell32 -lole32 -ladvapi32 -ldwmapi -static -mwindows -O2
#include <dwmapi.h>
#include <commctrl.h>
#include <shellapi.h>
// #include <mmsystem.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
// #include <random>
#include <thread>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

#define WM_TRAYNOTIFY (WM_APP + 1)

#pragma comment(lib, "comctl32.lib")
// #pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

// ======================== 自定义 JSON ========================
namespace SimpleJSON {
    enum class ValueType { Null, Bool, Number, String, Object, Array };

    class Value {
    public:
        ValueType type = ValueType::Null;
        bool boolVal = false;
        double numVal = 0.0;
        std::wstring strVal;
        std::map<std::wstring, Value> objVal;
        std::vector<Value> arrVal;

        Value() : type(ValueType::Null) {}
        Value(bool b) : type(ValueType::Bool), boolVal(b) {}
        Value(double d) : type(ValueType::Number), numVal(d) {}
        Value(const std::wstring& s) : type(ValueType::String), strVal(s) {}
        Value(const std::map<std::wstring, Value>& o) : type(ValueType::Object), objVal(o) {}
        Value(const std::vector<Value>& a) : type(ValueType::Array), arrVal(a) {}

        bool isNull() const { return type == ValueType::Null; }
        bool isBool() const { return type == ValueType::Bool; }
        bool isNumber() const { return type == ValueType::Number; }
        bool isString() const { return type == ValueType::String; }
        bool isObject() const { return type == ValueType::Object; }
        bool isArray() const { return type == ValueType::Array; }

        // 访问器
        bool asBool() const { return boolVal; }
        double asNumber() const { return numVal; }
        std::wstring asString() const { return strVal; }
        const std::map<std::wstring, Value>& asObject() const { return objVal; }
        const std::vector<Value>& asArray() const { return arrVal; }

        // 获取键值（对象）
        Value& operator[](const std::wstring& key) { return objVal[key]; }
        const Value& operator[](const std::wstring& key) const { return objVal.at(key); }
    };

    // ---------- 解析器 ----------
    class Parser {
    public:
        Parser(const std::wstring& input) : m_input(input), m_pos(0) {}

        Value parse() {
            skipWhitespace();
            Value v = parseValue();
            skipWhitespace();
            if (m_pos < m_input.size()) {
                // 忽略多余字符（容错）
            }
            return v;
        }

    private:
        std::wstring m_input;
        size_t m_pos;

        void skipWhitespace() {
            while (m_pos < m_input.size() && std::iswspace(m_input[m_pos])) m_pos++;
        }

        wchar_t peek() { return m_pos < m_input.size() ? m_input[m_pos] : 0; }
        wchar_t get() { return m_pos < m_input.size() ? m_input[m_pos++] : 0; }

        Value parseValue() {
            wchar_t c = peek();
            if (c == L'n') return parseNull();
            if (c == L't' || c == L'f') return parseBool();
            if (c == L'"') return parseString();
            if (c == L'{') return parseObject();
            if (c == L'[') return parseArray();
            if (c == L'-' || std::iswdigit(c)) return parseNumber();
            throw std::runtime_error("Unexpected token in JSON");
        }

        Value parseNull() {
            std::wstring word;
            for (int i = 0; i < 4; i++) word.push_back(get());
            if (word != L"null") throw std::runtime_error("Invalid null");
            return Value();
        }

        Value parseBool() {
            wchar_t c = get();
            if (c == L't') {
                for (int i = 0; i < 3; i++) get(); // rue
                return Value(true);
            } else {
                for (int i = 0; i < 4; i++) get(); // alse
                return Value(false);
            }
        }

        Value parseString() {
            get(); // consume "
            std::wstring result;
            while (true) {
                wchar_t c = get();
                if (c == L'"') break;
                if (c == L'\\') {
                    wchar_t esc = get();
                    switch (esc) {
                        case L'"': result.push_back(L'"'); break;
                        case L'\\': result.push_back(L'\\'); break;
                        case L'/': result.push_back(L'/'); break;
                        case L'b': result.push_back(L'\b'); break;
                        case L'f': result.push_back(L'\f'); break;
                        case L'n': result.push_back(L'\n'); break;
                        case L'r': result.push_back(L'\r'); break;
                        case L't': result.push_back(L'\t'); break;
                        case L'u': {
                            // 简单处理：只支持基本多语言平面，忽略代理对
                            wchar_t code = 0;
                            for (int i = 0; i < 4; i++) {
                                wchar_t h = get();
                                if (h >= L'0' && h <= L'9') code = code * 16 + (h - L'0');
                                else if (h >= L'a' && h <= L'f') code = code * 16 + (h - L'a' + 10);
                                else if (h >= L'A' && h <= L'F') code = code * 16 + (h - L'A' + 10);
                            }
                            result.push_back((wchar_t)code);
                            break;
                        }
                        default: result.push_back(esc);
                    }
                } else {
                    result.push_back(c);
                }
            }
            return Value(result);
        }

        Value parseNumber() {
            std::wstring num;
            if (peek() == L'-') num.push_back(get());
            while (std::iswdigit(peek())) num.push_back(get());
            if (peek() == L'.') {
                num.push_back(get());
                while (std::iswdigit(peek())) num.push_back(get());
            }
            if (peek() == L'e' || peek() == L'E') {
                num.push_back(get());
                if (peek() == L'+' || peek() == L'-') num.push_back(get());
                while (std::iswdigit(peek())) num.push_back(get());
            }
            double val = std::wcstod(num.c_str(), nullptr);
            return Value(val);
        }

        Value parseObject() {
            get(); // consume '{'
            std::map<std::wstring, Value> obj;
            skipWhitespace();
            if (peek() == L'}') { get(); return Value(obj); }
            while (true) {
                skipWhitespace();
                if (peek() != L'"') throw std::runtime_error("Expected key string");
                std::wstring key = parseString().asString();
                skipWhitespace();
                if (get() != L':') throw std::runtime_error("Expected colon");
                skipWhitespace();
                Value val = parseValue();
                obj[key] = val;
                skipWhitespace();
                wchar_t c = peek();
                if (c == L'}') { get(); break; }
                if (c == L',') { get(); continue; }
                throw std::runtime_error("Expected comma or closing brace");
            }
            return Value(obj);
        }

        Value parseArray() {
            get(); // consume '['
            std::vector<Value> arr;
            skipWhitespace();
            if (peek() == L']') { get(); return Value(arr); }
            while (true) {
                skipWhitespace();
                Value val = parseValue();
                arr.push_back(val);
                skipWhitespace();
                wchar_t c = peek();
                if (c == L']') { get(); break; }
                if (c == L',') { get(); continue; }
                throw std::runtime_error("Expected comma or closing bracket");
            }
            return Value(arr);
        }
    };

    // 辅助：宽字符转 UTF-8
    std::string WideToAnsi(const std::wstring& wstr) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
        str.pop_back();
        return str;
    }

    // ---------- 序列化 ----------
    std::wstring stringify(const Value& v, int indent = 0) {
        std::wstring result;
        switch (v.type) {
            case ValueType::Null: result = L"null"; break;
            case ValueType::Bool: result = v.boolVal ? L"true" : L"false"; break;
            case ValueType::Number: {
                wchar_t buf[64];
                swprintf(buf, 64, L"%.16g", v.numVal);
                result = buf;
                break;
            }
            case ValueType::String: {
                result = L"\"";
                for (wchar_t c : v.strVal) {
                    if (c == L'\"') result += L"\\\"";
                    else if (c == L'\\') result += L"\\\\";
                    else if (c == L'\b') result += L"\\b";
                    else if (c == L'\f') result += L"\\f";
                    else if (c == L'\n') result += L"\\n";
                    else if (c == L'\r') result += L"\\r";
                    else if (c == L'\t') result += L"\\t";
                    else if (c < 0x20) {
                        wchar_t buf[8];
                        swprintf(buf, 8, L"\\u%04x", c);
                        result += buf;
                    } else result.push_back(c);
                }
                result += L"\"";
                break;
            }
            case ValueType::Object: {
                result = L"{";
                bool first = true;
                for (const auto& pair : v.objVal) {
                    if (!first) result += L",";
                    if (indent >= 0) result += L"\n" + std::wstring(indent + 2, L' ');
                    result += L"\"" + pair.first + L"\":" + (indent >= 0 ? L" " : L"") + stringify(pair.second, indent + 2);
                    first = false;
                }
                if (indent >= 0 && !v.objVal.empty()) result += L"\n" + std::wstring(indent, L' ');
                result += L"}";
                break;
            }
            case ValueType::Array: {
                result = L"[";
                bool first = true;
                for (const auto& item : v.arrVal) {
                    if (!first) result += L",";
                    if (indent >= 0) result += L"\n" + std::wstring(indent + 2, L' ');
                    result += stringify(item, indent + 2);
                    first = false;
                }
                if (indent >= 0 && !v.arrVal.empty()) result += L"\n" + std::wstring(indent, L' ');
                result += L"]";
                break;
            }
        }
        return result;
    }

    // 从文件读取 JSON
    Value parseFile(const std::wstring& path) {
        std::string utf8Path = WideToAnsi(path);
        std::ifstream f(utf8Path, std::ios::binary);
        if (!f.is_open()) return Value(); // null
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        // 转为宽字符（假定 UTF-8）
        int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wstr[0], len);
        wstr.pop_back(); // 移除结尾 null
        Parser parser(wstr);
        return parser.parse();
    }

    // 写入文件
    void writeFile(const std::wstring& path, const Value& v) {
        std::wstring str = stringify(v, 2); // 缩进 2 空格
        std::string utf8 = WideToAnsi(str);
        std::string utf8Path = WideToAnsi(path);
        std::ofstream f(utf8Path, std::ios::binary);
        if (f.is_open()) f.write(utf8.c_str(), utf8.size());
    }
} // namespace SimpleJSON

// ======================== 全局设置 ========================
using namespace SimpleJSON;

std::wstring g_lang = L"zh";
int g_workDuration = 1200;
int g_restDuration = 20;
bool g_useOverlay = true;
bool g_requireCode = false;
bool g_useWarning = true;
std::wstring g_warnPosition = L"pos_br";
std::wstring g_hotkeyPause = L"ctrl+shift+p";
std::wstring g_hotkeySkip = L"ctrl+shift+s";
std::wstring g_theme = L"Dark";
std::wstring g_fontFamily = L"Microsoft YaHei UI";
int g_totalCycles = 0;
bool g_autoStart = false;
std::wstring g_overlayMode = L"fullscreen";

std::wstring g_basePath;

// ======================== 多语言 ========================
std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> translations = {
    {L"zh", {
        {L"title", L"守护双眼👁️"},
        {L"focus", L"下次提醒"},
        {L"pause", L"已暂停"},
        {L"rest_overlay", L"远眺休息！"},
        {L"rest_no_overlay", L"休息（无弹窗）"},
        {L"btn_pause", L"暂停"},
        {L"btn_resume", L"恢复"},
        {L"btn_skip", L"跳过本轮"},
        {L"btn_settings", L"设置"},
        {L"info_hotkeys", L"暂停：{pause} | 跳过：{skip}"},
        {L"cycles_count", L"🔥 已完成周期数：{count}"},
        {L"overlay_main", L"您已持续用眼过久\n休息一会吧！\n请将注意力集中在至少 6 米远的地方！"},
        {L"overlay_time", L"剩余：{time} 秒"},
        {L"warning_text", L"即将休息"},
        {L"btn_rest_now", L"立即休息"},
        {L"tray_show", L"显示主界面"},
        {L"tray_restart", L"重启程序"},
        {L"tray_quit", L"彻底退出"},
        {L"tray_tip", L"守护双眼"},
        {L"set_press_hk", L"按下快捷键组合..."},
    }},
    {L"en", {
        {L"title", L"Save Your Peepers 👁️"},
        {L"focus", L"Focus"},
        {L"pause", L"Paused"},
        {L"rest_overlay", L"Look away!"},
        {L"rest_no_overlay", L"Rest (No screen)"},
        {L"btn_pause", L"Pause"},
        {L"btn_resume", L"Resume"},
        {L"btn_skip", L"Skip Cycle"},
        {L"btn_settings", L"Settings"},
        {L"info_hotkeys", L"Pause: {pause} | Skip: {skip}"},
        {L"cycles_count", L"🔥 Completed cycles: {count}"},
        {L"overlay_main", L"Look away from the screen!"},
        {L"overlay_time", L"{time} sec left"},
        {L"warning_text", L"👀 Rest soon!"},
        {L"btn_rest_now", L"Rest Now"},
        {L"tray_show", L"Show"},
        {L"tray_restart", L"Restart"},
        {L"tray_quit", L"Quit"},
        {L"tray_tip", L"Save Your Peepers Timer"},
        {L"set_press_hk", L"Press combination..."},
    }},
    {L"ru", {
        {L"title", L"Защитите свои глаза👁️"},
        {L"focus", L"Сосредоточьтесь"},
        {L"pause", L"Приостановлено"},
        {L"rest_overlay", L"Смотрите вдаль!"},
        {L"rest_no_overlay", L"Отдых (без экрана)"},
        {L"btn_pause", L"Пауза"},
        {L"btn_resume", L"Возобновить"},
        {L"btn_skip", L"Пропустить цикл"},
        {L"btn_settings", L"Настройки"},
        {L"info_hotkeys", L"Пауза: {pause} | Пропуск: {skip}"},
        {L"cycles_count", L"🔥 Завершено циклов: {count}"},
        {L"overlay_main", L"Вы слишком долго смотрите в экран\nОтдохните!\nСмотрите вдаль (не менее 6 метров)!"},
        {L"overlay_time", L"Осталось: {time} сек."},
        {L"warning_text", L"👀 Скоро отдых!"},
        {L"btn_rest_now", L"Отдохнуть сейчас"},
        {L"tray_show", L"Показать главное окно"},
        {L"tray_restart", L"Перезапустить"},
        {L"tray_quit", L"Выйти"},
        {L"tray_tip", L"Таймер для глаз"},
        {L"set_press_hk", L"Нажмите комбинацию клавиш..."},
    }}
};

std::wstring tr(const std::wstring& key) {
    auto& langMap = translations[g_lang];
    if (langMap.find(key) != langMap.end()) return langMap[key];
    return key;
}

// ======================== 工具函数 ========================
std::wstring GetBasePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    size_t pos = dir.find_last_of(L'\\');
    if (pos != std::wstring::npos) dir = dir.substr(0, pos);
    return dir;
}

std::wstring GetSettingsFile() {
    return g_basePath + L"\\settings.json";
}



void LoadSettings() {
    std::wstring path = GetSettingsFile();
    if (!std::filesystem::exists(path)) return;
    Value root = parseFile(path);
    if (root.isNull()) return;
    if (root.isObject()) {
        if (root[L"lang"].isString()) g_lang = root[L"lang"].asString();
        if (root[L"work_duration"].isNumber()) g_workDuration = (int)root[L"work_duration"].asNumber();
        if (root[L"rest_duration"].isNumber()) g_restDuration = (int)root[L"rest_duration"].asNumber();
        if (root[L"use_overlay"].isBool()) g_useOverlay = root[L"use_overlay"].asBool();
        if (root[L"require_code"].isBool()) g_requireCode = root[L"require_code"].asBool();
        if (root[L"use_warning"].isBool()) g_useWarning = root[L"use_warning"].asBool();
        if (root[L"warn_position"].isString()) g_warnPosition = root[L"warn_position"].asString();
        if (root[L"hotkey_pause"].isString()) g_hotkeyPause = root[L"hotkey_pause"].asString();
        if (root[L"hotkey_skip"].isString()) g_hotkeySkip = root[L"hotkey_skip"].asString();
        if (root[L"theme"].isString()) g_theme = root[L"theme"].asString();
        if (root[L"font_name"].isString()) g_fontFamily = root[L"font_name"].asString();
        if (root[L"total_cycles"].isNumber()) g_totalCycles = (int)root[L"total_cycles"].asNumber();
        if (root[L"auto_start"].isBool()) g_autoStart = root[L"auto_start"].asBool();
        if (root[L"overlay_mode"].isString()) g_overlayMode = root[L"overlay_mode"].asString();
    }
}

void SaveSettings() {
    Value root;
    root.type = ValueType::Object;
    root[L"lang"] = Value(g_lang);
    root[L"work_duration"] = Value((double)g_workDuration);
    root[L"rest_duration"] = Value((double)g_restDuration);
    root[L"use_overlay"] = Value(g_useOverlay);
    root[L"require_code"] = Value(g_requireCode);
    root[L"use_warning"] = Value(g_useWarning);
    root[L"warn_position"] = Value(g_warnPosition);
    root[L"hotkey_pause"] = Value(g_hotkeyPause);
    root[L"hotkey_skip"] = Value(g_hotkeySkip);
    root[L"theme"] = Value(g_theme);
    root[L"font_name"] = Value(g_fontFamily);
    root[L"total_cycles"] = Value((double)g_totalCycles);
    root[L"auto_start"] = Value(g_autoStart);
    root[L"overlay_mode"] = Value(g_overlayMode);
    writeFile(GetSettingsFile(), root);
}

std::wstring FormatTime(int seconds) {
    wchar_t buf[16];
    wsprintfW(buf, L"%02d:%02d", seconds / 60, seconds % 60);
    return std::wstring(buf);
}

void PlayRandomSound() {
    std::wstring exePath = g_basePath + L"\\audio_player.exe";
    if (!std::filesystem::exists(exePath)) return;   // 静默忽略，不播放音频

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        // 不等待进程结束，立即关闭句柄
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// ======================== 窗口类 ========================
class TimerApp {
public:
    TimerApp() : m_hWnd(nullptr), m_hWndWarn(nullptr), m_isPaused(false), m_isWorking(true),
                 m_timeLeft(g_workDuration), m_overlayProcess(nullptr), m_hFontNormal(nullptr), m_hFontBig(nullptr), m_hWarnFont(nullptr), m_hWarnBgBrush(nullptr), m_hBgBrush(nullptr),
                 m_hoveredBtn(nullptr)
    {
        // 初始化鼠标跟踪映射
        m_btnTracking[nullptr] = false;
        // 根据主题设置颜色
        if (g_theme == L"Dark") {
            m_bgColor = RGB(32, 32, 32);
            m_textColor = RGB(240, 240, 240);
        } else {
            m_bgColor = RGB(255, 255, 255);
            m_textColor = RGB(0, 0, 0);
        }
        m_hBgBrush = CreateSolidBrush(m_bgColor);
    }

    bool Create() {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(GetModuleHandle(nullptr), L"MAINICON");
        wc.hbrBackground = m_hBgBrush;
        wc.lpszClassName = L"TimerAppClass";
        RegisterClassExW(&wc);

        m_hWnd = CreateWindowExW(0, L"TimerAppClass", tr(L"title").c_str(),
                                 WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
                                 CW_USEDEFAULT, CW_USEDEFAULT, 540, 680,
                                 nullptr, nullptr, wc.hInstance, this);
        if (!m_hWnd) return false;

        // 深色标题栏
        BOOL darkTitle = (g_theme == L"Dark") ? TRUE : FALSE;
        DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                              &darkTitle, sizeof(BOOL));

        CreateControls();
        RegisterHotKey(m_hWnd, 1, MOD_CONTROL | MOD_SHIFT, 'P');
        RegisterHotKey(m_hWnd, 2, MOD_CONTROL | MOD_SHIFT, 'S');
        SetTimer(m_hWnd, 1001, 1000, nullptr);
        AddTrayIcon();
        UpdateUI();
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);
        return true;
    }

    ~TimerApp() {
        if (m_overlayProcess) {
            TerminateProcess(m_overlayProcess, 0);
            CloseHandle(m_overlayProcess);
        }
        RemoveTrayIcon();
        if (m_hWndWarn) DestroyWindow(m_hWndWarn);
        
        // 释放 GDI 资源
        if (m_hFontNormal) DeleteObject(m_hFontNormal);
        if (m_hFontBig) DeleteObject(m_hFontBig);
        if (m_hWarnFont) DeleteObject(m_hWarnFont);
        if (m_hWarnBgBrush) DeleteObject(m_hWarnBgBrush);
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
    }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        TimerApp* app = nullptr;
        if (msg == WM_CREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            app = reinterpret_cast<TimerApp*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)app);
        } else {
            app = reinterpret_cast<TimerApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }
        if (app) return app->HandleMessage(hWnd, msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

private:
    HWND m_hWnd, m_hWndWarn;
    void RestartApp() {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOW);
        PostQuitMessage(0);
    }

    void ToggleWindowVisibility() {
        if (IsWindowVisible(m_hWnd)) {
            ShowWindow(m_hWnd, SW_HIDE);
        } else {
            ShowWindow(m_hWnd, SW_SHOW);
            SetForegroundWindow(m_hWnd);
        }
    }
    HWND m_hPhaseLabel, m_hTimeLabel, m_hPauseBtn, m_hSkipBtn, m_hRestNowBtn, m_hSettingsBtn, m_hCyclesLabel, m_hInfoLabel;
    bool m_isPaused, m_isWorking;
    int m_timeLeft;
    HANDLE m_overlayProcess;
    HFONT m_hFontNormal = nullptr;
    HFONT m_hFontBig = nullptr;
    HFONT m_hWarnFont = nullptr;
    HBRUSH m_hWarnBgBrush = nullptr;
    COLORREF m_bgColor, m_textColor;   // 背景色和文本色
    HBRUSH m_hBgBrush;                 // 对应背景色的画刷
    std::unordered_map<HWND, bool> m_buttonHover;   // 按钮→是否悬停
    HWND m_hoveredBtn = nullptr;                    // 当前悬停的按钮句柄
    std::unordered_map<HWND, bool> m_btnTracking;   // 每个按钮的鼠标跟踪状态
    // 原 WndProc 保存
    std::unordered_map<HWND, WNDPROC> m_oldBtnProcs;
    // 子类化窗口过程（静态）
    static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    // 处理子类化消息的辅助函数
    LRESULT OnButtonMouse(HWND hBtn, UINT msg, WPARAM wParam, LPARAM lParam);

    void DrawButton(LPDRAWITEMSTRUCT dis) {
        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        bool dark = (g_theme == L"Dark");
        bool hover = false;
        auto it = m_buttonHover.find(dis->hwndItem);
        if (it != m_buttonHover.end())
            hover = it->second;

        COLORREF bg;
        if (hover)
        {
            // 悬停时：深色主题用更亮的灰，浅色主题用更暗的灰
            bg = dark ? RGB(80, 80, 80) : RGB(200, 200, 200);
        }
        else
        {
            bg = dark ? RGB(45, 45, 45) : RGB(250, 250, 250);
        }
        // 按下时（ODS_SELECTED）可保持原有逻辑，或与悬停叠加
        if (dis->itemState & ODS_SELECTED)
        {
            bg = dark ? RGB(70, 70, 70) : RGB(220, 220, 220);
        }

        COLORREF fg = dark ? RGB(240, 240, 240) : RGB(30, 30, 30);
        COLORREF border = dark ? RGB(100, 100, 100) : RGB(180, 180, 180);

        // 背景
        HBRUSH brush = CreateSolidBrush(bg);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        // 边框
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        // 文字
        wchar_t text[256];
        GetWindowTextW(dis->hwndItem, text, 256);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, fg);
        SelectObject(hdc, m_hFontNormal);
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void CreateControls() {
        m_hFontNormal = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                   L"Microsoft YaHei UI");

        m_hPhaseLabel = CreateWindowW(L"STATIC", tr(L"focus").c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_CENTER,
                                     10, 30, 520, 50, m_hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hPhaseLabel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

        m_hTimeLabel = CreateWindowW(L"STATIC", FormatTime(m_timeLeft).c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_CENTER,
                                     10, 90, 520, 90, m_hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        m_hFontBig = CreateFontW(-60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                 L"Microsoft YaHei UI");
        SendMessage(m_hTimeLabel, WM_SETFONT, (WPARAM)m_hFontBig, TRUE);

        m_hPauseBtn = CreateWindowW(L"BUTTON", tr(L"btn_pause").c_str(),
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                    120, 200, 300, 50, m_hWnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hPauseBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        m_buttonHover[m_hPauseBtn] = false;
        m_btnTracking[m_hPauseBtn] = false;
        SetWindowLongPtrW(m_hPauseBtn, GWLP_USERDATA, (LONG_PTR)this);
        m_oldBtnProcs[m_hPauseBtn] = (WNDPROC)SetWindowLongPtrW(m_hPauseBtn, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);

        m_hSkipBtn = CreateWindowW(L"BUTTON", tr(L"btn_skip").c_str(),
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                   120, 260, 300, 50, m_hWnd, (HMENU)11, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hSkipBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        m_buttonHover[m_hSkipBtn] = false;
        m_btnTracking[m_hSkipBtn] = false;
        SetWindowLongPtrW(m_hSkipBtn, GWLP_USERDATA, (LONG_PTR)this);
        m_oldBtnProcs[m_hSkipBtn] = (WNDPROC)SetWindowLongPtrW(m_hSkipBtn, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);

        m_hRestNowBtn = CreateWindowW(L"BUTTON", tr(L"btn_rest_now").c_str(),
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                      120, 320, 300, 50, m_hWnd, (HMENU)12, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hRestNowBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        m_buttonHover[m_hRestNowBtn] = false;
        m_btnTracking[m_hRestNowBtn] = false;
        SetWindowLongPtrW(m_hRestNowBtn, GWLP_USERDATA, (LONG_PTR)this);
        m_oldBtnProcs[m_hRestNowBtn] = (WNDPROC)SetWindowLongPtrW(m_hRestNowBtn, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);

        m_hSettingsBtn = CreateWindowW(L"BUTTON", tr(L"btn_settings").c_str(),
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                       120, 380, 300, 50, m_hWnd, (HMENU)13, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hSettingsBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        m_buttonHover[m_hSettingsBtn] = false;
        m_btnTracking[m_hSettingsBtn] = false;
        SetWindowLongPtrW(m_hSettingsBtn, GWLP_USERDATA, (LONG_PTR)this);
        m_oldBtnProcs[m_hSettingsBtn] = (WNDPROC)SetWindowLongPtrW(m_hSettingsBtn, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);

        m_hCyclesLabel = CreateWindowW(L"STATIC", L"",
                                       WS_CHILD | WS_VISIBLE | SS_CENTER,
                                       10, 480, 520, 40, m_hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hCyclesLabel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

        m_hInfoLabel = CreateWindowW(L"STATIC", L"",
                                     WS_CHILD | WS_VISIBLE | SS_CENTER,
                                     10, 530, 520, 40, m_hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        SendMessage(m_hInfoLabel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    }

    void UpdateUI() {
        std::wstring phase = m_isPaused ? tr(L"pause") : (m_isWorking ? tr(L"focus") : (g_useOverlay ? tr(L"rest_overlay") : tr(L"rest_no_overlay")));
        SetWindowTextW(m_hPhaseLabel, phase.c_str());
        SetWindowTextW(m_hTimeLabel, FormatTime(m_timeLeft).c_str());
        SetWindowTextW(m_hPauseBtn, m_isPaused ? tr(L"btn_resume").c_str() : tr(L"btn_pause").c_str());

        std::wstring cycles = tr(L"cycles_count");
        size_t pos = cycles.find(L"{count}");
        if (pos != std::wstring::npos) cycles.replace(pos, 7, std::to_wstring(g_totalCycles));
        SetWindowTextW(m_hCyclesLabel, cycles.c_str());

        std::wstring info = tr(L"info_hotkeys");
        size_t p1 = info.find(L"{pause}");
        if (p1 != std::wstring::npos) info.replace(p1, 7, g_hotkeyPause);
        size_t p2 = info.find(L"{skip}");
        if (p2 != std::wstring::npos) info.replace(p2, 6, g_hotkeySkip);
        SetWindowTextW(m_hInfoLabel, info.c_str());
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case 10: TogglePause(); break;
                    case 11: SkipCycle(); break;
                    case 12: RestNow(); break;
                    case 13: OpenSettings(); break;
                    case 1001: // 显示主界面
                        ShowWindow(m_hWnd, SW_SHOW);
                        SetForegroundWindow(m_hWnd);
                        break;
                    case 1003:
                        RestartApp();
                        break;
                    case 1002: // 彻底退出
                        DestroyWindow(m_hWnd);   // 会触发 WM_DESTROY，随后 PostQuitMessage
                        break;
                }
                break;
            case WM_TIMER:
                if (wParam == 1001) OnTimer();
                else if (wParam == 1002) { HideWarning(); KillTimer(m_hWnd, 1002); }
                break;
            case WM_HOTKEY:
                if (wParam == 1) TogglePause();
                else if (wParam == 2) SkipCycle();
                break;
            case WM_SYSCOMMAND:
                if (wParam == SC_CLOSE) { HideToTray(); return 0; }
                // 其他系统命令（最小化、移动、调整大小等）交给默认处理
                return DefWindowProcW(hWnd, msg, wParam, lParam);
            case WM_CTLCOLORSTATIC:
            {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, m_textColor);
                SetBkColor(hdc, m_bgColor);
                return (LRESULT)m_hBgBrush;
            }
            case WM_CTLCOLORBTN:
            {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, m_textColor);
                SetBkColor(hdc, m_bgColor);
                return (LRESULT)m_hBgBrush;
            }
            case WM_DRAWITEM: {
                DrawButton((LPDRAWITEMSTRUCT)lParam);
                return TRUE;
            }
            case WM_MOUSEMOVE:
            {
                // 获取鼠标在客户区的位置
                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                // 获取该位置下的子窗口（最顶层）
                HWND hChild = ChildWindowFromPoint(m_hWnd, pt);

                // 检查该子窗口是否是我们关心的按钮
                auto it = m_buttonHover.find(hChild);
                if (it != m_buttonHover.end())
                {
                    // 鼠标落在某个按钮上
                    if (m_hoveredBtn != hChild)
                    {
                        // 清除旧悬停
                        if (m_hoveredBtn && m_buttonHover.find(m_hoveredBtn) != m_buttonHover.end())
                        {
                            m_buttonHover[m_hoveredBtn] = false;
                            InvalidateRect(m_hoveredBtn, nullptr, TRUE);
                        }
                        // 设置新悬停
                        m_hoveredBtn = hChild;
                        m_buttonHover[hChild] = true;
                        InvalidateRect(hChild, nullptr, TRUE);
                    }
                }
                else
                {
                    // 鼠标不在任何按钮上
                    if (m_hoveredBtn)
                    {
                        if (m_buttonHover.find(m_hoveredBtn) != m_buttonHover.end())
                        {
                            m_buttonHover[m_hoveredBtn] = false;
                            InvalidateRect(m_hoveredBtn, nullptr, TRUE);
                        }
                        m_hoveredBtn = nullptr;
                    }
                }

                // 启用鼠标离开跟踪（只调用一次）- 保持全局跟踪以捕获窗口级别的离开
                if (m_btnTracking.count(nullptr) == 0 || !m_btnTracking[nullptr])
                {
                    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd, 0 };
                    TrackMouseEvent(&tme);
                    m_btnTracking[nullptr] = true;
                }
                return 0;
            }
            case WM_MOUSELEAVE:
            {
                m_btnTracking[nullptr] = false;
                if (m_hoveredBtn)
                {
                    if (m_buttonHover.find(m_hoveredBtn) != m_buttonHover.end())
                    {
                        m_buttonHover[m_hoveredBtn] = false;
                        InvalidateRect(m_hoveredBtn, nullptr, TRUE);
                    }
                    m_hoveredBtn = nullptr;
                }
                return 0;
            }
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            case WM_TRAYNOTIFY:
                if (lParam == WM_LBUTTONDBLCLK) {
                    ShowWindow(m_hWnd, SW_SHOW);
                    SetForegroundWindow(m_hWnd);
                } else if (lParam == WM_RBUTTONUP) {
                    ShowTrayMenu();
                } else if (lParam == WM_LBUTTONUP) {
                    ToggleWindowVisibility();
                }
                break;
            default:
                return DefWindowProcW(hWnd, msg, wParam, lParam);
        }
        return 0;
    }

    void OnTimer() {
        if (m_isPaused) return;

        // 检查外部进程结束
        if (!m_isWorking && m_overlayProcess) {
            DWORD ret = WaitForSingleObject(m_overlayProcess, 0);
            if (ret == WAIT_OBJECT_0) {
                CloseHandle(m_overlayProcess);
                m_overlayProcess = nullptr;
                g_totalCycles++;
                SaveSettings();
                m_isWorking = true;
                m_timeLeft = g_workDuration;
                UpdateUI();
            }
            return;
        }

        if (m_timeLeft > 0) {
            m_timeLeft--;
            if (m_isWorking && g_useWarning && m_timeLeft == 10) {
                PlayRandomSound();
                ShowWarning();
            }
            if (m_timeLeft == 0) {
                if (m_isWorking) {
                    m_isWorking = false;
                    m_timeLeft = g_restDuration;
                    StartOverlayProcess();
                    UpdateUI();
                }
            }
            SetWindowTextW(m_hTimeLabel, FormatTime(m_timeLeft).c_str());
        }
    }

    void StartOverlayProcess() {
        std::wstring exeName = (g_overlayMode == L"securedesktop") ? L"securedesktop.exe" : L"overlay.exe";
        std::wstring exePath = g_basePath + L"\\" + exeName;
        if (!std::filesystem::exists(exePath)) {
            exePath = g_basePath + L"\\overlay.exe";
            if (!std::filesystem::exists(exePath)) return;
        }
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            m_overlayProcess = pi.hProcess;
        }
    }

    void ShowWarning() {
        if (m_hWndWarn && IsWindow(m_hWndWarn)) return;
        // 先以极小尺寸创建窗口
        m_hWndWarn = CreateWindowW(L"STATIC", tr(L"warning_text").c_str(),
                                   WS_POPUP | WS_VISIBLE | SS_CENTER,
                                   0, 0, 1, 1, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        SetWindowLongW(m_hWndWarn, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
        SetWindowLongW(m_hWndWarn, GWL_STYLE, WS_POPUP | WS_VISIBLE | SS_CENTER);

        m_hWarnBgBrush = CreateSolidBrush(RGB(255, 255, 255));

        // 创建字体并设置到窗口
        m_hWarnFont = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Microsoft YaHei UI");
        SendMessage(m_hWndWarn, WM_SETFONT, (WPARAM)m_hWarnFont, TRUE);
        SetLayeredWindowAttributes(m_hWndWarn, 0, 180, LWA_ALPHA);

        // 计算文字实际尺寸
        HDC hdc = GetDC(m_hWndWarn);
        if (hdc) {
            SelectObject(hdc, m_hWarnFont);
            std::wstring text = tr(L"warning_text");
            SIZE size;
            GetTextExtentPoint32W(hdc, text.c_str(), (int)text.length(), &size);
            ReleaseDC(m_hWndWarn, hdc);

            int padding = 0;                        // 窗口大小和文字大小一致
            int width  = size.cx + padding * 2;
            int height = size.cy + padding * 2;

            // 根据预设位置重新计算坐标
            RECT workArea;
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
            int sw = workArea.right - workArea.left;
            int sh = workArea.bottom - workArea.top;
            int x, y;
            if (g_warnPosition == L"pos_br") { x = sw - width - 20;  y = sh - height - 20; }
            else if (g_warnPosition == L"pos_bl") { x = 20;           y = sh - height - 20; }
            else if (g_warnPosition == L"pos_tr") { x = sw - width - 20; y = 20; }
            else if (g_warnPosition == L"pos_tl") { x = 20;           y = 20; }
            else if (g_warnPosition == L"pos_center") { x = (sw - width)/2;  y = (sh - height)/2; }
            else if (g_warnPosition == L"pos_top_center") { x = (sw - width)/2;  y = 20; }
            else if (g_warnPosition == L"pos_bottom_center") { x = (sw - width)/2;  y = sh - height - 20; }
            else { x = sw - width - 20;  y = sh - height - 20; }   // 默认右下

            SetWindowPos(m_hWndWarn, HWND_TOPMOST, x, y, width, height,
                         SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        }

        SetClassLongPtrW(m_hWndWarn, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(WHITE_BRUSH));
        ShowWindow(m_hWndWarn, SW_SHOW);
        UpdateWindow(m_hWndWarn);

        SetTimer(m_hWnd, 1002, 10000, nullptr);
    }

    void HideWarning() {
        if (m_hWndWarn && IsWindow(m_hWndWarn)) {
            DestroyWindow(m_hWndWarn);
            m_hWndWarn = nullptr;
        }
    }

    NOTIFYICONDATAW m_nid;
    void AddTrayIcon() {
        ZeroMemory(&m_nid, sizeof(m_nid));
        m_nid.cbSize = sizeof(m_nid);
        m_nid.hWnd = m_hWnd;
        m_nid.uID = 100;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYNOTIFY;
        HICON hIcon = LoadIconW(GetModuleHandle(nullptr), L"MAINICON");
        if (!hIcon) hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
        m_nid.hIcon = hIcon;
        std::wstring tip = tr(L"tray_tip");
        wcscpy_s(m_nid.szTip, tip.c_str());
        Shell_NotifyIconW(NIM_ADD, &m_nid);
    }

    void RemoveTrayIcon() { Shell_NotifyIconW(NIM_DELETE, &m_nid); }

    void ShowTrayMenu() {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, tr(L"tray_show").c_str());
        AppendMenuW(hMenu, MF_STRING, 1003, tr(L"tray_restart").c_str());
        AppendMenuW(hMenu, MF_STRING, 1002, tr(L"tray_quit").c_str());
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(m_hWnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
        PostMessage(m_hWnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }

    void HideToTray() { ShowWindow(m_hWnd, SW_HIDE); }

    void TogglePause() { m_isPaused = !m_isPaused; UpdateUI(); }

    void SkipCycle() {
        if (m_overlayProcess) {
            TerminateProcess(m_overlayProcess, 0);
            CloseHandle(m_overlayProcess);
            m_overlayProcess = nullptr;
        }
        m_isWorking = true;
        m_timeLeft = g_workDuration;
        HideWarning();
        UpdateUI();
    }

    void RestNow() {
        if (m_isPaused || !m_isWorking) return;
        HideWarning();
        m_isWorking = false;
        m_timeLeft = g_restDuration;
        StartOverlayProcess();
        UpdateUI();
    }

    void OpenSettings() {
        std::wstring exePath = g_basePath + L"\\Settings.exe";
        if (std::filesystem::exists(exePath))
            ShellExecuteW(nullptr, L"open", exePath.c_str(), nullptr, nullptr, SW_SHOW);
        else
            MessageBoxW(m_hWnd, L"未找到 Settings.exe", L"错误", MB_OK);
    }
};

// ======================== 子类化实现 ========================
// 子类化窗口过程
LRESULT CALLBACK TimerApp::ButtonSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TimerApp* app = (TimerApp*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    if (app) {
        // 如果是鼠标相关消息，交给 OnButtonMouse 处理
        if (msg == WM_MOUSEMOVE || msg == WM_MOUSELEAVE) {
            return app->OnButtonMouse(hWnd, msg, wParam, lParam);
        }
        // 其他消息调用原窗口过程
        WNDPROC oldProc = app->m_oldBtnProcs[hWnd];
        if (oldProc) return CallWindowProcW(oldProc, hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// 处理子类化消息的辅助函数
LRESULT TimerApp::OnButtonMouse(HWND hBtn, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE: {
            // 若此前未跟踪，启用离开跟踪
            if (!m_btnTracking[hBtn]) {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hBtn, 0 };
                TrackMouseEvent(&tme);
                m_btnTracking[hBtn] = true;
            }
            // 如果当前不是悬停状态，设置并重绘
            if (!m_buttonHover[hBtn]) {
                m_buttonHover[hBtn] = true;
                InvalidateRect(hBtn, nullptr, TRUE);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            m_btnTracking[hBtn] = false;
            if (m_buttonHover[hBtn]) {
                m_buttonHover[hBtn] = false;
                InvalidateRect(hBtn, nullptr, TRUE);
            }
            break;
        }
    }
    // 调用原按钮过程，让按钮正常响应点击等
    WNDPROC oldProc = m_oldBtnProcs[hBtn];
    if (oldProc) return CallWindowProcW(oldProc, hBtn, msg, wParam, lParam);
    return DefWindowProcW(hBtn, msg, wParam, lParam);
}

// ======================== WinMain ========================

// ======================== WinMain ========================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 高DPI感知
    SetProcessDPIAware();
    // 或者使用 SetProcessDPIAwarenessContext(PROCESS_PER_MONITOR_DPI_AWARE) 但需要 Win10 1703+

    g_basePath = GetBasePath();

    LoadSettings();

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    TimerApp app;
    if (!app.Create()) {
        MessageBoxW(nullptr, L"创建窗口失败", L"错误", MB_OK);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}