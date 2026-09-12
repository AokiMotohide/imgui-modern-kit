#pragma once
#include <windows.h>
#include <filesystem>
#include <string>

namespace imkit::gallery {
struct NodeLauncher {
    HANDLE process = nullptr;
    DWORD processId = 0, error = 0;
    ~NodeLauncher() { if (process) CloseHandle(process); }
    void Open() {
        if (process && WaitForSingleObject(process, 0) == WAIT_TIMEOUT) {
            EnumWindows([](HWND window, LPARAM id) -> BOOL {
                DWORD owner = 0;
                GetWindowThreadProcessId(window, &owner);
                if (owner == static_cast<DWORD>(id) && IsWindowVisible(window)) {
                    if (IsIconic(window)) ShowWindow(window, SW_RESTORE);
                    SetForegroundWindow(window);
                    return FALSE;
                }
                return TRUE;
            }, static_cast<LPARAM>(processId));
            return;
        }
        if (process) { CloseHandle(process); process = nullptr; }
        wchar_t path[32768]{};
        if (!GetModuleFileNameW(nullptr, path, DWORD(std::size(path)))) {
            error = GetLastError();
            return;
        }
        auto executable = std::filesystem::path(path).parent_path() / L"imkit_node_editor_gallery.exe";
        std::wstring command = L"\"" + executable.wstring() + L"\" --studio";
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION info{};
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
                            nullptr, executable.parent_path().c_str(), &startup, &info)) {
            error = GetLastError();
            return;
        }
        process = info.hProcess;
        processId = info.dwProcessId;
        error = 0;
        CloseHandle(info.hThread);
    }
};
}
