// audio_player.cpp — 播放一次随机音频后退出（无窗口、无控制台）
// 编译（Release）：
//   g++ -o audio_player.exe audio_player.cpp -lwinmm -static -mwindows -O2

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

// ========== 以下为从原代码中直接复制的音频逻辑（未作任何改动） ==========

static std::wstring g_lastPlayedSound;
static bool         g_hasLastPlayed = false;

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

bool PlayRandomSound(bool waitForFinish = false) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
        dir = dir.substr(0, lastSlash + 1);
    std::wstring soundsFolder = dir + L"sounds";

    DWORD attr = GetFileAttributesW(soundsFolder.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        return false;

    auto allFiles = GetSoundFiles(soundsFolder);
    if (allFiles.empty())
        return false;

    // 候选列表（避免连续播放同一首，但仅播放一次时此逻辑不影响结果）
    std::vector<std::wstring> candidates = allFiles;
    if (g_hasLastPlayed && allFiles.size() > 1) {
        auto it = std::find(candidates.begin(), candidates.end(), g_lastPlayedSound);
        if (it != candidates.end())
            candidates.erase(it);
    }
    if (candidates.empty())
        candidates = allFiles;

    int idx = rand() % candidates.size();
    std::wstring selected = candidates[idx];
    g_lastPlayedSound = selected;
    g_hasLastPlayed = true;

    int volume = 600 + (rand() % 401);

    mciSendStringW(L"close SoundAlias", nullptr, 0, nullptr);

    std::wstring cmdOpen = L"open \"" + selected + L"\" alias SoundAlias";
    if (mciSendStringW(cmdOpen.c_str(), nullptr, 0, nullptr) != 0)
        return false;

    wchar_t volCmd[256];
    swprintf_s(volCmd, L"setaudio SoundAlias volume to %d", volume);
    mciSendStringW(volCmd, nullptr, 0, nullptr);

    std::wstring cmdPlay = L"play SoundAlias from 0";
    if (waitForFinish) cmdPlay += L" wait";
    if (mciSendStringW(cmdPlay.c_str(), nullptr, 0, nullptr) != 0) {
        mciSendStringW(L"close SoundAlias", nullptr, 0, nullptr);
        return false;
    }
    if (waitForFinish) {
        mciSendStringW(L"close SoundAlias", nullptr, 0, nullptr);
    }
    return true;
}

// ========== Windows 无窗口入口 ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 初始化随机种子（使用系统时间）
    srand(static_cast<unsigned>(time(nullptr)));

    // 同步播放一次（等待播放完成）
    PlayRandomSound(true);

    // 播放完毕，直接退出进程
    return 0;
}