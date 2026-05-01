#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")

#define IDC_STATIC_FILENAME 1001
#define IDC_STATIC_STATUS 1002
#define IDC_BUTTON_ADD 1003
#define IDC_BUTTON_REMOVE 1004

std::wstring GetSelfPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

std::wstring GetSelfDirectory() {
    std::wstring path = GetSelfPath();
    size_t pos = path.find_last_of(L'\\');
    if (pos != std::wstring::npos) {
        return path.substr(0, pos);
    }
    return L"";
}

std::wstring FindAutostartBat(const std::wstring& dir) {
    std::wstring batPath = dir + L"\\Autostart.bat";
    
    DWORD fileAttributes = GetFileAttributesW(batPath.c_str());
    if (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return batPath;
    }
    
    return L"";
}

bool IsInAutostart(const std::wstring& appName) {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_READ, &hKey);
    
    if (status != ERROR_SUCCESS) {
        return false;
    }
    
    wchar_t value[MAX_PATH];
    DWORD size = MAX_PATH * sizeof(wchar_t);
    status = RegQueryValueExW(hKey, appName.c_str(), NULL, NULL, 
        reinterpret_cast<LPBYTE>(value), &size);
    
    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

bool AddToAutostart(const std::wstring& appName, const std::wstring& exePath) {
    if (exePath.empty()) {
        return false;
    }
    
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_SET_VALUE, &hKey);
    
    if (status != ERROR_SUCCESS) {
        return false;
    }
    
    std::wstring value = L"\"" + exePath + L"\"";
    status = RegSetValueExW(hKey, appName.c_str(), 0, REG_SZ, 
        reinterpret_cast<const BYTE*>(value.c_str()), 
        (value.size() + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

bool RemoveFromAutostart(const std::wstring& appName) {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_SET_VALUE, &hKey);
    
    if (status != ERROR_SUCCESS) {
        return false;
    }
    
    status = RegDeleteValueW(hKey, appName.c_str());
    RegCloseKey(hKey);
    
    return (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
}

std::wstring GetFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L'\\');
    if (pos != std::wstring::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

int GetScaledSize(int size) {
    HDC hdc = GetDC(NULL);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
    return MulDiv(size, dpi, 96);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hStaticFileName, hStaticStatus, hBtnAdd, hBtnRemove;
    static std::wstring exePath;
    static std::wstring appName = L"SaveYourPeepers_守护你的眼睛";
    
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            std::wstring* pPath = static_cast<std::wstring*>(pCreate->lpCreateParams);
    
            if (pPath) {
                exePath = *pPath;
                delete pPath;
            }

            HINSTANCE hInst = pCreate->hInstance;
            
            int scaledX = GetScaledSize(20);
            int scaledY = GetScaledSize(20);
            int scaledWidth = GetScaledSize(280);
            int scaledFileNameHeight = GetScaledSize(40);
            int scaledStatusHeight = GetScaledSize(25);
            int scaledButtonWidth = GetScaledSize(130);
            int scaledButtonHeight = GetScaledSize(30);
            
            hStaticFileName = CreateWindowW(L"STATIC", L"", 
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                scaledX, scaledY, scaledWidth, scaledFileNameHeight, hwnd, (HMENU)IDC_STATIC_FILENAME, hInst, NULL);
            
            hStaticStatus = CreateWindowW(L"STATIC", L"", 
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                scaledX, scaledY + scaledFileNameHeight + GetScaledSize(10), 
                scaledWidth, scaledStatusHeight, hwnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);
            
            hBtnAdd = CreateWindowW(L"BUTTON", L"添加到开机自启动", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                scaledX, scaledY + scaledFileNameHeight + scaledStatusHeight + GetScaledSize(30), 
                scaledButtonWidth, scaledButtonHeight, hwnd, (HMENU)IDC_BUTTON_ADD, hInst, NULL);
            
            hBtnRemove = CreateWindowW(L"BUTTON", L"从开机自启动移除", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                scaledX + scaledButtonWidth + GetScaledSize(20), 
                scaledY + scaledFileNameHeight + scaledStatusHeight + GetScaledSize(30), 
                scaledButtonWidth, scaledButtonHeight, hwnd, (HMENU)IDC_BUTTON_REMOVE, hInst, NULL);
            
            if (exePath.empty()) {
                SetWindowTextW(hStaticFileName, L"错误: 未找到Autostart.bat文件");
                SetWindowTextW(hStaticStatus, L"状态: 无法检测");
                EnableWindow(hBtnAdd, FALSE);
                EnableWindow(hBtnRemove, FALSE);
            } else {
                std::wstring fileName = GetFileName(exePath);
                std::wstring fileText = L"检测到的文件: " + fileName;
                SetWindowTextW(hStaticFileName, fileText.c_str());
                
                bool isInAutostart = IsInAutostart(appName);
                std::wstring status = isInAutostart ? L"状态: 已添加到开机自启动" : L"状态: 未添加到开机自启动";
                SetWindowTextW(hStaticStatus, status.c_str());
            }
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_BUTTON_ADD: {
                    if (exePath.empty()) {
                        MessageBoxW(hwnd, L"未找到Autostart.bat文件，无法添加", L"错误", MB_OK | MB_ICONERROR);
                        break;
                    }
                    
                    if (AddToAutostart(appName, exePath)) {
                        SetWindowTextW(hStaticStatus, L"状态: 已添加到开机自启动");
                        MessageBoxW(hwnd, L"成功添加到开机自启动", L"成功", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(hwnd, L"添加失败，请检查权限或文件路径", L"错误", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
                
                case IDC_BUTTON_REMOVE: {
                    if (RemoveFromAutostart(appName)) {
                        SetWindowTextW(hStaticStatus, L"状态: 已从开机自启动移除");
                        MessageBoxW(hwnd, L"成功从开机自启动移除", L"成功", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(hwnd, L"移除失败，可能未添加到自启动", L"错误", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
            }
            return 0;
        }
        
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();
    
    std::wstring dir = GetSelfDirectory();
    std::wstring exePath = FindAutostartBat(dir);
    
    if (exePath.empty()) {
        MessageBoxW(NULL, L"未找到Autostart.bat文件", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    const wchar_t CLASS_NAME[] = L"AutostartManagerClass";
    
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassExW(&wc);
    
    int scaledWindowWidth = GetScaledSize(400);
    int scaledWindowHeight = GetScaledSize(240);
    
    HWND hwnd = CreateWindowExW(
        WS_EX_WINDOWEDGE | WS_EX_TOPMOST,
        CLASS_NAME,
        L"开机自启动管理器",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, scaledWindowWidth, scaledWindowHeight,
        NULL, NULL, hInstance, reinterpret_cast<LPVOID>(new std::wstring(exePath)))
    ;
    
    if (!hwnd) {
        return 1;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return 0;
}