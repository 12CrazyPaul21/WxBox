#include <utils/common.h>

//
// Functions
//

#if WXBOX_IN_WINDOWS_OS

static inline std::vector<wb_process::ProcessInfo> GetProcessList_Windows()
{
    std::vector<wb_process::ProcessInfo> vt;
    HANDLE                               hSnapshot             = NULL;
    HANDLE                               hProcess              = NULL;
    PROCESSENTRY32                       pe32                  = {0};
    char                                 fullPath[MAX_PATH]    = {0};
    char                                 absFullPath[MAX_PATH] = {0};

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return vt;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
    }

    do {
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
        if (!hProcess) {
            continue;
        }

        if (!::GetModuleFileNameExA(hProcess, NULL, fullPath, MAX_PATH)) {
            CloseHandle(hProcess);
            continue;
        }
        CloseHandle(hProcess);

        if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
            continue;
        }

        wb_process::ProcessInfo pi;
        pi.abspath  = absFullPath;
        pi.filename = pe32.szExeFile;
        pi.dirpath  = std::move(wxbox::util::file::ToDirectoryPath(absFullPath));
        pi.pid      = pe32.th32ProcessID;
        vt.push_back(std::move(pi));
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return std::move(vt);
}

static bool GetModuleInfo_Windows(wb_process::PID pid, const std::string& moduleName, wb_process::ModuleInfo& moduleInfo)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    if (Module32First(hSnap, &modEntry)) {
        do {
            if (!::_stricmp(modEntry.szModule, moduleName.c_str())) {
                moduleInfo.hModule         = modEntry.hModule;
                moduleInfo.moduleName      = modEntry.szModule;
                moduleInfo.modulePath      = modEntry.szExePath;
                moduleInfo.pModuleBaseAddr = modEntry.modBaseAddr;
                moduleInfo.uModuleSize     = modEntry.modBaseSize;
                return true;
            }

        } while (Module32Next(hSnap, &modEntry));
    }

    return false;
}

static inline wb_process::PID StartProcessAndAttach_Windows(const std::string& binFilePath)
{
    BOOL                status = FALSE;
    PROCESS_INFORMATION pi     = {0};
    STARTUPINFOA        si     = {0};

    si.cb  = sizeof(si);
    status = ::CreateProcessA(binFilePath.c_str(),
                              nullptr,
                              nullptr,
                              nullptr,
                              FALSE,
                              CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS,
                              nullptr,
                              nullptr,
                              &si,
                              &pi);
    if (!status) {
        return 0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return pi.dwProcessId;
}

#elif WXBOX_IN_MAC_OS

static inline std::vector<wb_process::ProcessInfo> GetProcessList_Mac()
{
    std::vector<wb_process::ProcessInfo> vt;
    return std::move(vt);
}

static inline bool GetModuleInfo_Mac(wb_process::PID pid, const std::string& moduleName, wb_process::ModuleInfo& moduleInfo)
{
    return false;
}

static inline wb_process::PID StartProcessAndAttach_Mac(const std::string& binFilePath)
{
    return 0;
}

#endif

std::vector<wb_process::ProcessInfo> wxbox::util::process::GetProcessList()
{
#if WXBOX_IN_WINDOWS_OS
    return std::move(GetProcessList_Windows());
#elif WXBOX_IN_MAC_OS
    return std::move(GetProcessList_Mac());
#endif
}

wxbox::util::process::PID wxbox::util::process::GetCurrentProcessId()
{
#if WXBOX_IN_WINDOWS_OS
    return ::GetCurrentProcessId();
#elif WXBOX_IN_MAC_OS
    return getpid();
#endif
}

wxbox::util::process::PROCESS_HANDLE wxbox::util::process::OpenProcessHandle(PID pid)
{
#if WXBOX_IN_WINDOWS_OS
    return ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
#elif WXBOX_IN_MAC_OS
    return 0;
#endif
}

void wxbox::util::process::CloseProcessHandle(PROCESS_HANDLE handle)
{
#if WXBOX_IN_WINDOWS_OS
    CloseHandleSafe(handle);
#elif WXBOX_IN_MAC_OS
    
#endif
}

wb_process::WIN_HANDLE wxbox::util::process::GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt)
{
#if WXBOX_IN_WINDOWS_OS
    return (WIN_HANDLE)::WindowFromPoint((POINT)pt);
#elif WXBOX_IN_MAC_OS
    return nullptr;
#endif
}

bool wxbox::util::process::GetProcessInfoFromWindowHandle(const WIN_HANDLE& hWnd, ProcessInfo& pi)
{
    if (!hWnd) {
        return false;
    }

#if WXBOX_IN_WINDOWS_OS

    DWORD pid = 0;
    ::GetWindowThreadProcessId((HWND)hWnd, &pid);
    if (!pid) {
        return false;
    }

    return wb_process::GetProcessInfoByPID(pid, pi);

#elif WXBOX_IN_MAC_OS
    return false;
#endif
}

bool wxbox::util::process::GetModuleInfo(PID pid, const std::string& moduleName, ModuleInfo& moduleInfo)
{
#if WXBOX_IN_WINDOWS_OS
    return GetModuleInfo_Windows(pid, moduleName, moduleInfo);
#elif WXBOX_IN_MAC_OS
    return GetModuleInfo_Mac(pid, moduleName, moduleInfo);
#endif
}

bool wxbox::util::process::GetProcessInfoByPID(PID pid, ProcessInfo& pi)
{
#if WXBOX_IN_WINDOWS_OS

    HANDLE hProcess              = NULL;
    char   fullPath[MAX_PATH]    = {0};
    char   absFullPath[MAX_PATH] = {0};

    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        return false;
    }

    if (!::GetModuleFileNameExA(hProcess, NULL, fullPath, MAX_PATH)) {
        CloseHandle(hProcess);
        return false;
    }
    CloseHandle(hProcess);

    if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
        return false;
    }

    pi.abspath  = absFullPath;
    pi.filename = ::PathFindFileNameA(absFullPath);
    pi.dirpath  = std::move(wxbox::util::file::ToDirectoryPath(absFullPath));
    pi.pid      = pid;

    return true;

#elif WXBOX_IN_MAC_OS
    return false;
#endif
}

wb_process::PID wxbox::util::process::StartProcessAndAttach(const std::string& binFilePath)
{
#if WXBOX_IN_WINDOWS_OS
    return StartProcessAndAttach_Windows(binFilePath);
#elif WXBOX_IN_MAC_OS
    return StartProcessAndAttach_Mac(binFilePath);
#endif
}