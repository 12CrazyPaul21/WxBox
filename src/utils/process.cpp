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

        if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
            CloseHandle(hProcess);
            continue;
        }

		wb_process::ProcessInfo pi;
        pi.abspath  = absFullPath;
        pi.filename = pe32.szExeFile;
        pi.dirpath  = std::move(wxbox::util::file::ToDirectoryPath(absFullPath));
        pi.pid      = pe32.th32ProcessID;
        vt.push_back(std::move(pi));

		CloseHandle(hProcess);
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
    // all 32 bits processes in Windows Platform
    return std::move(GetProcessList_Windows());
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    return std::move(GetProcessList_Mac());
#endif
}