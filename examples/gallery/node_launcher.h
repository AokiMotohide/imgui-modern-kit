#pragma once
#include <filesystem>
#include <string>
#include <cerrno>
#ifdef _WIN32
#include <windows.h>
#else
#include <mach-o/dyld.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace imkit::gallery {
struct NodeLauncher {
#ifdef _WIN32
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
#else
    pid_t process = -1;
    int error = 0;
    void Open() {
        if(process>0 && waitpid(process,nullptr,WNOHANG)==0) return;
        std::uint32_t size=0; _NSGetExecutablePath(nullptr,&size);
        std::string buffer(size,'\0');
        if(_NSGetExecutablePath(buffer.data(),&size)!=0) { error=EINVAL; return; }
        auto bundleDirectory=std::filesystem::weakly_canonical(buffer).parent_path();
        for(int i=0;i<3;++i) bundleDirectory=bundleDirectory.parent_path();
        const auto executable=bundleDirectory/"imkit_node_editor_gallery.app"/"Contents"/"MacOS"/"imkit_node_editor_gallery";
        const std::string path=executable.string();
        char *arguments[]={const_cast<char*>(path.c_str()),const_cast<char*>("--studio"),nullptr};
        error=posix_spawn(&process,path.c_str(),nullptr,nullptr,arguments,environ);
        if(error) process=-1;
    }
#endif
};
}
