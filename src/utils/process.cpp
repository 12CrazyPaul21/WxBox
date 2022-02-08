#include <utils/common.h>

//
// AppSingleton
//

wb_process::AppSingleton::AppSingleton(const std::string& name, bool lock)
  : name(name)
  , mutexName("")
  , winIdSharedName("")
  , winId(nullptr)
  , mutex(nullptr)
  , shared(nullptr)
{
    mutexName       = "____" + name + "_Singleton_Mutex____";
    winIdSharedName = "____" + name + "_Singleton_WinId_Shared____";

    if (!lock) {
        return;
    }

    if (!TryLock()) {
        WakeApplicationWindow();
        std::exit(0);
    }
}

wb_process::AppSingleton::~AppSingleton()
{
    Release();
}

bool wb_process::AppSingleton::TryLock()
{
    if (mutex) {
        return false;
    }

#if WXBOX_IN_WINDOWS_OS
    auto hMutex = ::CreateMutexA(NULL, TRUE, mutexName.c_str());
    if (!hMutex) {
        return false;
    }

    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandleSafe(hMutex);
        return false;
    }

    mutex = hMutex;
    return true;
#else
    throw std::exception("AppSingleton::TryLock stub");
    return false;
#endif
}

void wb_process::AppSingleton::Release()
{
#if WXBOX_IN_WINDOWS_OS
    if (mutex) {
        ::CloseHandle(mutex);
        mutex = nullptr;
    }

    if (winId) {
        ::UnmapViewOfFile(winId);
        winId = nullptr;
    }

    if (shared) {
        ::CloseHandle(shared);
        shared = nullptr;
    }

#else
    throw std::exception("AppSingleton::Release stub");
#endif
}

void wb_process::AppSingleton::RecordWindowId(ucpulong_t id)
{
#if WXBOX_IN_WINDOWS_OS
    shared = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*winId), winIdSharedName.c_str());
    if (!shared) {
        return;
    }

    winId = (ucpulong_t*)::MapViewOfFile(shared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*winId));
    if (!winId) {
        ::CloseHandle(shared);
        shared = nullptr;
        return;
    }

    *winId = id;
#else
    throw std::exception("AppSingleton::RecordWindowId stub");
#endif
}

void wb_process::AppSingleton::WakeApplicationWindow()
{
#if WXBOX_IN_WINDOWS_OS

    static constexpr UINT WM_XSTYLE_WAKE_UP = WM_USER + 0x502;

    // open file mapping
    HANDLE hShared = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, winIdSharedName.c_str());
    if (!hShared) {
        return;
    }

    // map winId
    ucpulong_t* pWinId = (ucpulong_t*)::MapViewOfFile(hShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ucpulong_t));
    if (!pWinId) {
        ::CloseHandle(hShared);
        return;
    }

    // get winId
    HWND hWnd = (HWND)*pWinId;
    ::UnmapViewOfFile(pWinId);
    ::CloseHandle(hShared);

    // wake singleton application window
    // ShowWindow(hWnd, SW_SHOWNORMAL);
    // SetActiveWindow(hWnd);
    // SetForegroundWindow(hWnd);
    PostMessageA(hWnd, WM_XSTYLE_WAKE_UP, 0, 0);

#else
    throw std::exception("AppSingleton::WakeApplicationWindow stub");
#endif
}

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

    hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return vt;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!::Process32First(hSnapshot, &pe32)) {
        ::CloseHandle(hSnapshot);
        return vt;
    }

    do {
        hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
        if (!hProcess) {
            continue;
        }

        if (!::GetModuleFileNameExA(hProcess, NULL, fullPath, MAX_PATH)) {
            ::CloseHandle(hProcess);
            continue;
        }
        ::CloseHandle(hProcess);

        if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
            continue;
        }

        wb_process::ProcessInfo pi;
        pi.abspath  = absFullPath;
        pi.filename = pe32.szExeFile;
        pi.dirpath  = wxbox::util::file::ToDirectoryPath(absFullPath);
        pi.pid      = pe32.th32ProcessID;
        vt.emplace_back(std::move(pi));
    } while (::Process32Next(hSnapshot, &pe32));

    ::CloseHandle(hSnapshot);
    return vt;
}

static bool Is64Process_Windows(wb_process::PROCESS_HANDLE hProcess)
{
    using FnIsWow64Process = BOOL(WINAPI*)(HANDLE hProcess, PBOOL Wow64Process);

    if (!wb_platform::Is64System()) {
        return false;
    }

    if (!hProcess) {
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        return false;
    }

    FnIsWow64Process fnIsWow64Process = (FnIsWow64Process)GetProcAddress(hKernel32, "IsWow64Process");
    if (!fnIsWow64Process) {
        return false;
    }

    BOOL isWow64Process = FALSE;
    if (!fnIsWow64Process(hProcess, &isWow64Process)) {
        return false;
    }

    return !isWow64Process;
}

static bool GetModuleInfo_Windows(wb_process::PID pid, const std::string& moduleName, wb_process::ModuleInfo& moduleInfo)
{
    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    if (::Module32First(hSnapshot, &modEntry)) {
        do {
            if (!::_stricmp(modEntry.szModule, moduleName.c_str())) {
                moduleInfo.hModule         = modEntry.hModule;
                moduleInfo.moduleName      = modEntry.szModule;
                moduleInfo.modulePath      = modEntry.szExePath;
                moduleInfo.pModuleBaseAddr = modEntry.modBaseAddr;
                moduleInfo.uModuleSize     = modEntry.modBaseSize;
                ::CloseHandle(hSnapshot);
                return true;
            }

        } while (::Module32Next(hSnapshot, &modEntry));
    }

    ::CloseHandle(hSnapshot);
    return false;
}

static std::vector<wb_process::ModuleInfo> CollectModuleInfos_Windows(wb_process::PID pid)
{
    std::vector<wb_process::ModuleInfo> results;

    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return results;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    if (::Module32First(hSnapshot, &modEntry)) {
        do {
            wb_process::ModuleInfo moduleInfo;
            moduleInfo.hModule         = modEntry.hModule;
            moduleInfo.moduleName      = modEntry.szModule;
            moduleInfo.modulePath      = modEntry.szExePath;
            moduleInfo.pModuleBaseAddr = modEntry.modBaseAddr;
            moduleInfo.uModuleSize     = modEntry.modBaseSize;
            results.emplace_back(std::move(moduleInfo));
        } while (::Module32Next(hSnapshot, &modEntry));
    }

    ::CloseHandle(hSnapshot);
    return results;
}

static inline wb_process::PID StartProcess_Windows(const std::string& binFilePath, bool isAttach)
{
    BOOL                status = FALSE;
    PROCESS_INFORMATION pi     = {0};
    STARTUPINFOA        si     = {0};
    DWORD               dwFlag = CREATE_NEW_CONSOLE;

    if (isAttach) {
        dwFlag |= DEBUG_ONLY_THIS_PROCESS;
    }

    si.cb  = sizeof(si);
    status = ::CreateProcessA(binFilePath.c_str(),
                              nullptr,
                              nullptr,
                              nullptr,
                              FALSE,
                              dwFlag,
                              nullptr,
                              nullptr,
                              &si,
                              &pi);
    if (!status) {
        return 0;
    }

    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return pi.dwProcessId;
}

static bool SuspendAllOtherThread_Windows(wb_process::PID pid, wb_process::TID tid)
{
    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(threadEntry);

    if (!::Thread32First(hSnapshot, &threadEntry)) {
        ::CloseHandle(hSnapshot);
        return false;
    }

    do {
        if (threadEntry.th32OwnerProcessID == pid && threadEntry.th32ThreadID != tid) {
            HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
            if (hThread) {
                ::SuspendThread(hThread);
                ::CloseHandle(hThread);
            }
        }

    } while (::Thread32Next(hSnapshot, &threadEntry));

    ::CloseHandle(hSnapshot);
    return true;
    return false;
}

#elif WXBOX_IN_MAC_OS

static inline std::vector<wb_process::ProcessInfo> GetProcessList_Mac()
{
    std::vector<wb_process::ProcessInfo> vt;
    throw std::exception("GetProcessList_Mac stub");
    return vt;
}

static bool Is64Process_Mac(wb_process::PROCESS_HANDLE hProcess)
{
    throw std::exception("Is64Process_Mac stub");
    return false;
}

static inline bool GetModuleInfo_Mac(wb_process::PID pid, const std::string& moduleName, wb_process::ModuleInfo& moduleInfo)
{
    throw std::exception("GetModuleInfo_Mac stub");
    return false;
}

static std::vector<wb_process::ModuleInfo> CollectModuleInfos_Mac(wb_process::PID pid)
{
    throw std::exception("CollectModuleInfos_Mac stub");
    return std::vector<wb_process::ModuleInfo>();
}

static inline wb_process::PID StartProcess_Mac(const std::string& binFilePath, bool isAttach)
{
    throw std::exception("StartProcess_Mac stub");
    return 0;
}

static bool SuspendAllOtherThread_Mac(wb_process::PID pid, wb_process::TID tid)
{
    throw std::exception("SuspendAllOtherThread_Mac stub");
    return false;
}

#endif

std::time_t wxbox::util::process::GetCurrentTimestamp(bool ms)
{
    auto t = std::chrono::system_clock::now().time_since_epoch();

    if (ms) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
    }
    else {
        return std::chrono::duration_cast<std::chrono::seconds>(t).count();
    }
}

std::vector<wb_process::ProcessInfo> wxbox::util::process::GetProcessList()
{
#if WXBOX_IN_WINDOWS_OS
    return GetProcessList_Windows();
#elif WXBOX_IN_MAC_OS
    return GetProcessList_Mac();
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

wxbox::util::process::PROCESS_HANDLE wxbox::util::process::GetCurrentProcessHandle()
{
#if WXBOX_IN_WINDOWS_OS
    return ::GetCurrentProcess();
#elif WXBOX_IN_MAC_OS
    throw std::exception("GetCurrentProcessHandle stub");
    return 0;
#endif
}

wxbox::util::process::TID wxbox::util::process::GetCurrentThreadId()
{
#if WXBOX_IN_WINDOWS_OS
    return ::GetCurrentThreadId();
#elif WXBOX_IN_MAC_OS
    return gettid();
#endif
}

wxbox::util::process::THREAD_HANDLE wxbox::util::process::GetCurrentThreadHandle()
{
#if WXBOX_IN_WINDOWS_OS
    return ::GetCurrentThread();
#elif WXBOX_IN_MAC_OS
    throw std::exception("GetCurrentThreadHandle stub");
    return 0;
#endif
}

bool wxbox::util::process::Is64Process(PROCESS_HANDLE hProcess)
{
#if WXBOX_IN_WINDOWS_OS
    return Is64Process_Windows(hProcess);
#elif WXBOX_IN_MAC_OS
    return Is64Process_Mac(hProcess);
#endif
}

wxbox::util::process::PROCESS_HANDLE wxbox::util::process::OpenProcessHandle(PID pid)
{
#if WXBOX_IN_WINDOWS_OS
    return ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
#elif WXBOX_IN_MAC_OS
    throw std::exception("OpenProcessHandle stub");
    return 0;
#endif
}

void wxbox::util::process::CloseProcessHandle(PROCESS_HANDLE handle)
{
#if WXBOX_IN_WINDOWS_OS
    CloseHandleSafe(handle);
#elif WXBOX_IN_MAC_OS
    throw std::exception("CloseProcessHandle stub");
#endif
}

wb_process::WIN_HANDLE wxbox::util::process::GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt)
{
#if WXBOX_IN_WINDOWS_OS
    return (WIN_HANDLE)::WindowFromPoint((POINT)pt);
#elif WXBOX_IN_MAC_OS
    throw std::exception("GetWindowHandleFromScreenPoint stub");
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
    throw std::exception("GetProcessInfoFromWindowHandle stub");
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

std::vector<wb_process::ModuleInfo> wxbox::util::process::CollectModuleInfos(PID pid)
{
#if WXBOX_IN_WINDOWS_OS
    return CollectModuleInfos_Windows(pid);
#elif WXBOX_IN_MAC_OS
    return CollectModuleInfos_Mac(pid);
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
    pi.dirpath  = wxbox::util::file::ToDirectoryPath(absFullPath);
    pi.pid      = pid;

    return true;

#elif WXBOX_IN_MAC_OS
    throw std::exception("GetProcessInfoByPID stub");
    return false;
#endif
}

wb_process::PID wxbox::util::process::StartProcess(const std::string& binFilePath, bool isAttach)
{
#if WXBOX_IN_WINDOWS_OS
    return StartProcess_Windows(binFilePath, isAttach);
#elif WXBOX_IN_MAC_OS
    return StartProcess_Mac(binFilePath, isAttach);
#endif
}

bool wxbox::util::process::SuspendAllOtherThread(PID pid, TID tid)
{
#if WXBOX_IN_WINDOWS_OS
    return SuspendAllOtherThread_Windows(pid, tid);
#elif WXBOX_IN_MAC_OS
    return SuspendAllOtherThread_Mac(pid, tid);
#endif
}

void wxbox::util::process::SetThreadName(THREAD_HANDLE hThread, const std::string& threadName)
{
    if (!hThread) {
        return;
    }

#if WXBOX_IN_WINDOWS_OS

    //
    // SetThreadDescription only for Windows 10 and above
    //

    using FnSetThreadDescription = HRESULT(WINAPI*)(HANDLE hThread, PCWSTR lpThreadDescription);

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        return;
    }

    FnSetThreadDescription fnSetThreadDescription = (FnSetThreadDescription)GetProcAddress(hKernel32, "SetThreadDescription");
    if (!fnSetThreadDescription) {
        return;
    }

    fnSetThreadDescription(hThread, wb_string::ToWString(threadName).c_str());

#elif WXBOX_IN_MAC_OS
    throw std::exception("SetThreadName stub");
#endif
}

std::string wxbox::util::process::GetThreadName(THREAD_HANDLE hThread)
{
    if (!hThread) {
        return "";
    }

#if WXBOX_IN_WINDOWS_OS

    //
    // GetThreadDescription only for Windows 10 and above
    //

    using FnGetThreadDescription = HRESULT(WINAPI*)(HANDLE hThread, PWSTR * ppszThreadDescription);

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        return "";
    }

    FnGetThreadDescription fnGetThreadDescription = (FnGetThreadDescription)GetProcAddress(hKernel32, "GetThreadDescription");
    if (!fnGetThreadDescription) {
        return "";
    }

    std::string threadName;
    wchar_t*    threadDescription = nullptr;

    if (SUCCEEDED(fnGetThreadDescription(hThread, &threadDescription))) {
        threadName = wb_string::ToString(threadDescription);
        ::LocalFree(threadDescription);
    }

    return threadName;

#elif WXBOX_IN_MAC_OS
    throw std::exception("GetThreadName stub");
    return "";
#endif
}