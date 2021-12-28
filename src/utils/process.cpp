#include <utils/common.h>

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

#elif WXBOX_IN_MAC_OS

static inline std::vector<wb_process::ProcessInfo> GetProcessList_Mac()
{
    std::vector<wb_process::ProcessInfo> vt;
    return std::move(vt);
}

#endif

std::vector<wb_process::ProcessInfo> wxbox::util::process::GetProcessList()
{
#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    return std::move(GetProcessList_Windows());
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    return std::move(GetProcessList_Mac());
#endif
}

wb_process::WIN_HANDLE wxbox::util::process::GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt)
{
#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    return (WIN_HANDLE)::WindowFromPoint((POINT)pt);
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    return nullptr;
#endif
}

bool wxbox::util::process::GetProcessInfoFromWindowHandle(const WIN_HANDLE& hWnd, ProcessInfo& pi)
{
    if (!hWnd) {
        return false;
    }

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    
	DWORD  pid                   = 0;
    HANDLE hProcess              = NULL;
    char   fullPath[MAX_PATH]    = {0};
    char   absFullPath[MAX_PATH] = {0};

    ::GetWindowThreadProcessId((HWND)hWnd, &pid);
    if (!pid) {
        return false;
    }

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

#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    return false;
#endif

    return true;
}