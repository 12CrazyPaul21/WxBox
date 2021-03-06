#include <utils/common.h>

#define WATCH_DOG_MAX_THREAD_COUNT 200

//
// Global Variables
//

wb_process::TID          g_watchDogTID            = 0;
std::atomic<std::time_t> g_watchDogCheckTimestamp = 0;
std::atomic<int>         g_watchDogFoundLockTimes = 0;
std::time_t              g_watchDogResumeInterval = 1000;
std::atomic<bool>        g_watchDogIsRunning      = false;
std::promise<void>       g_watchDogStopSignal;
std::promise<void>       g_watchDogExitSignal;
wb_process::TID          g_suspendThreadRecords[WATCH_DOG_MAX_THREAD_COUNT] = {0};
std::atomic<ucpulong_t>  g_suspendThreadRecordCount                         = 0;

//
// AutoProcessHandle
//

void wb_process::AutoProcessHandle::close()
{
    if (hProcess) {
        CloseProcessHandle(hProcess);
        hProcess = 0;
    }
}

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

static bool SuspendOrResumeAllThread_Windows(wb_process::PID pid, wb_process::TID tid, wb_process::TID watchDogTid, bool suspend)
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

    g_suspendThreadRecordCount = 0;

    do {
        if (threadEntry.th32OwnerProcessID == pid && threadEntry.th32ThreadID != tid && threadEntry.th32ThreadID != watchDogTid) {
            HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
            if (!hThread) {
                continue;
            }

            if (suspend) {
                ::SuspendThread(hThread);
                if (g_suspendThreadRecordCount < WATCH_DOG_MAX_THREAD_COUNT) {
                    g_suspendThreadRecords[g_suspendThreadRecordCount++] = threadEntry.th32ThreadID;
                }
            }
            else {
                ::ResumeThread(hThread);
            }

            ::CloseHandle(hThread);
        }

    } while (::Thread32Next(hSnapshot, &threadEntry));

    ::CloseHandle(hSnapshot);
    return true;
}

static bool SuspendAllOtherThread_Windows(wb_process::PID pid, wb_process::TID tid, wb_process::TID watchDogTid)
{
    return SuspendOrResumeAllThread_Windows(pid, tid, watchDogTid, true);
}

static void ResumeAllThread_Windows(wb_process::PID pid, wb_process::TID tid)
{
    SuspendOrResumeAllThread_Windows(pid, tid, 0, false);
}

std::vector<wb_process::TID, wb_memory::internal_allocator<wb_process::TID>> GetAllThreadId_Windows(wb_process::PID pid, wb_process::TID excludeThreadId, wb_process::TID watchDogThreadId)
{
    std::vector<wxbox::util::process::TID, wb_memory::internal_allocator<wxbox::util::process::TID>> result;

    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return result;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(threadEntry);

    if (!::Thread32First(hSnapshot, &threadEntry)) {
        ::CloseHandle(hSnapshot);
        return result;
    }

    do {
        if (threadEntry.th32OwnerProcessID == GetCurrentProcessId() && threadEntry.th32ThreadID != excludeThreadId && threadEntry.th32ThreadID != watchDogThreadId) {
            result.push_back(threadEntry.th32ThreadID);
        }

    } while (::Thread32Next(hSnapshot, &threadEntry));

    ::CloseHandle(hSnapshot);
    return result;
}

std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> WalkThreadStack_Windows(wb_process::TID tid)
{
    std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> result;

    if (!tid) {
        return result;
    }

    HANDLE hProcess = ::GetCurrentProcess();
    HANDLE hThread  = ::OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    if (!hThread) {
        return result;
    }

    STACKFRAME64 stackFrame;
    CONTEXT      context = {0};
    context.ContextFlags = CONTEXT_ALL;
    if (!GetThreadContext(hThread, &context)) {
        ::CloseHandle(hThread);
        return result;
    }

    std::memset(&stackFrame, 0, sizeof(stackFrame));

#if !defined(_AMD64_)
    DWORD machineType           = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrPC.Offset    = context.Eip;
    stackFrame.AddrStack.Mode   = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
#else
    DWORD machineType           = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrPC.Offset    = context.Rip;
    stackFrame.AddrStack.Mode   = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp;
#endif

    while (StackWalk64(machineType, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
#if !defined(_AMD64_)
        result.push_back(context.Eip);
#else
        result.push_back(context.Rip);
#endif
    }

    ::CloseHandle(hThread);
    return result;
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

static bool SuspendAllOtherThread_Mac(wb_process::PID pid, wb_process::TID tid, wb_process::TID watchDogTid)
{
    throw std::exception("SuspendAllOtherThread_Mac stub");
    return false;
}
static void ResumeAllThread_Mac(wb_process::PID pid, wb_process::TID tid)
{
    throw std::exception("SuspendAllOtherThread_Mac stub");
}

static std::vector<wb_process::TID, wb_memory::internal_allocator<wb_process::TID>> GetAllThreadId_Mac(wb_process::PID pid, wb_process::TID excludeThreadId, wb_process::TID watchDogThreadId)
{
    std::vector<wb_process::TID, wb_memory::internal_allocator<wb_process::TID>> result;
    throw std::exception("GetAllThreadId_Mac stub");
    return result;
}

static std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> WalkThreadStack_Mac(wb_process::TID tid)
{
    std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> result;
    throw std::exception("WalkThreadStack_Mac stub");
    return result;
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

bool wxbox::util::process::GetCurrentDate(struct std::tm& tm)
{
    bool result = true;

    try {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (localtime_s(&tm, &tt)) {
            result = false;
        }
    }
    catch (...) {
        result = false;
    }

    return result;
}

std::string wxbox::util::process::GetCurrentDateDesc()
{
    struct std::tm tm;
    if (!GetCurrentDate(tm)) {
        return "";
    }

    std::stringstream ss;

    try {
        ss << std::put_time(&tm, "%F %T");
    }
    catch (...) {
    }

    return ss.str();
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

wxbox::util::process::AutoProcessHandle wxbox::util::process::OpenProcessAutoHandle(PID pid)
{
    AutoProcessHandle handle;
    handle.hProcess = OpenProcessHandle(pid);
    return handle;
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

void wxbox::util::process::KillProcess(PID pid)
{
#if WXBOX_IN_WINDOWS_OS
    auto hProcess = OpenProcessAutoHandle(pid);
    if (hProcess.valid()) {
        TerminateProcess(hProcess.hProcess, 0);
    }
#elif WXBOX_IN_MAC_OS
    kill(pid, SIGKILL);
#endif
}

bool wxbox::util::process::SuspendAllOtherThread(PID pid, TID tid, TID watchDogTid)
{
#if WXBOX_IN_WINDOWS_OS
    return SuspendAllOtherThread_Windows(pid, tid, watchDogTid);
#elif WXBOX_IN_MAC_OS
    return SuspendAllOtherThread_Mac(pid, tid, watchDogTid);
#endif
}

void wxbox::util::process::ResumeAllThread(PID pid, TID tid)
{
    if (tid == 0) {
        tid = wb_process::GetCurrentThreadId();
    }

#if WXBOX_IN_WINDOWS_OS
    return ResumeAllThread_Windows(pid, tid);
#elif WXBOX_IN_MAC_OS
    return ResumeAllThread_Mac(pid, tid);
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

    fnSetThreadDescription(hThread, wb_string::ToNativeWString(threadName).c_str());

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
        threadName = wb_string::ToNativeString(threadDescription);
        ::LocalFree(threadDescription);
    }

    return threadName;

#elif WXBOX_IN_MAC_OS
    throw std::exception("GetThreadName stub");
    return "";
#endif
}

std::vector<wb_process::TID, wb_memory::internal_allocator<wb_process::TID>> wxbox::util::process::GetAllThreadId(PID pid, TID excludeThreadId, TID watchDogThreadId)
{
#if WXBOX_IN_WINDOWS_OS
    return GetAllThreadId_Windows(pid, excludeThreadId, watchDogThreadId);
#elif WXBOX_IN_MAC_OS
    return GetAllThreadId_Mac(pid, excludeThreadId, watchDogThreadId);
#endif
}

std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> wxbox::util::process::WalkThreadStack(TID tid)
{
#if WXBOX_IN_WINDOWS_OS
    return WalkThreadStack_Windows(tid);
#elif WXBOX_IN_MAC_OS
    return WalkThreadStack_Mac(tid);
#endif
}

std::set<ucpulong_t, std::less<int>, wb_memory::internal_allocator<ucpulong_t>> wxbox::util::process::GetAllOtherThreadCallFrameEips(wb_process::TID watchDogThreadId)
{
    std::set<ucpulong_t, std::less<int>, wb_memory::internal_allocator<ucpulong_t>> eips;
    for (auto tid : wb_process::GetAllThreadId(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId(), watchDogThreadId)) {
        auto frames = WalkThreadStack(tid);
        eips.insert(frames.begin(), frames.end());
    }
    return eips;
}

std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> wxbox::util::process::HitTestAllOtherThreadCallFrame(const CallFrameHitTestItemVector& targets, TID watchDogThreadId)
{
    std::vector<ucpulong_t, wb_memory::internal_allocator<ucpulong_t>> hit;

    auto eips = GetAllOtherThreadCallFrameEips(watchDogThreadId);
    if (eips.empty()) {
        return hit;
    }

    auto _targets = targets;

    for (auto eip : eips) {
        for (auto it = _targets.begin(); it != _targets.end();) {
            ucpulong_t begin_addr = (ucpulong_t)it->addr;
            ucpulong_t end_addr   = begin_addr + it->length;

            if (eip >= begin_addr && eip < end_addr) {
                hit.push_back(begin_addr);
                it = _targets.erase(it);
            }
            else {
                ++it;
            }
        }

        if (_targets.empty()) {
            goto _Finish;
        }
    }

_Finish:
    return hit;
}

bool wxbox::util::process::HitTestAllOtherThreadCallFrame(void* addr, ucpulong_t length, TID watchDogThreadId)
{
    auto eips = GetAllOtherThreadCallFrameEips(watchDogThreadId);
    if (eips.empty()) {
        return false;
    }

    ucpulong_t begin_addr = (ucpulong_t)addr;
    ucpulong_t end_addr   = begin_addr + length;

    for (auto eip : eips) {
        if (eip >= begin_addr && eip < end_addr) {
            return true;
        }
    }

    return false;
}

static void Inner_SuspendLockWatchDogRoutine_ResumeAllThread()
{
    ucpulong_t count = g_suspendThreadRecordCount.load();

    for (ucpulong_t i = 0; i < count; i++) {
#if WXBOX_IN_WINDOWS_OS
        HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, g_suspendThreadRecords[i]);
        if (hThread) {
            ::ResumeThread(hThread);
            ::CloseHandle(hThread);
        }
#else
        throw std::exception("Inner_SuspendLockWatchDogRoutine_ResumeAllThread stub");
#endif
    }
}

static void Inner_SuspendLockWatchDogRoutine()
{
    if (!g_watchDogIsRunning) {
        return;
    }

    try {
        auto stopFuture = g_watchDogStopSignal.get_future();
        g_watchDogCheckTimestamp.store(wb_process::GetCurrentTimestamp(true));
        g_watchDogFoundLockTimes.store(0);

        for (;;) {
            if (stopFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::timeout) {
                break;
            }

            // check interval
            auto timestamp = wb_process::GetCurrentTimestamp(true);
            if (timestamp - g_watchDogCheckTimestamp <= g_watchDogResumeInterval) {
                continue;
            }
            g_watchDogCheckTimestamp = timestamp;
            ++g_watchDogFoundLockTimes;

            // resume suspended threads
            Inner_SuspendLockWatchDogRoutine_ResumeAllThread();
        }

        g_watchDogExitSignal.set_value();
        g_watchDogIsRunning = false;
    }
    catch (...) {
    }
}

wb_process::TID wxbox::util::process::StartSuspendLockWatchDog(std::time_t intervalMs)
{
    std::promise<wb_process::TID> tidRequest;

    bool alreadyRunning = false;
    g_watchDogIsRunning.compare_exchange_strong(alreadyRunning, true);

    if (alreadyRunning) {
        return 0;
    }

    g_watchDogTID            = 0;
    g_watchDogStopSignal     = std::promise<void>();
    g_watchDogResumeInterval = intervalMs;

    try {
        auto watchDogThread = std::thread([intervalMs, &tidRequest] {
            tidRequest.set_value(wb_process::GetCurrentThreadId());
            Inner_SuspendLockWatchDogRoutine();
        });
        g_watchDogTID       = tidRequest.get_future().get();

        watchDogThread.detach();
    }
    catch (...) {
    }

    return g_watchDogTID;
}

void wxbox::util::process::TouchSuspendLockWatchDog()
{
    if (!g_watchDogIsRunning) {
        return;
    }

    g_watchDogCheckTimestamp.store(wb_process::GetCurrentTimestamp(true));
}

int wxbox::util::process::StopSuspendLockWatchDog()
{
    if (!g_watchDogIsRunning) {
        return 0;
    }

    try {
        g_watchDogStopSignal.set_value();
        g_watchDogExitSignal.get_future().wait();
    }
    catch (...) {
    }

    return g_watchDogFoundLockTimes;
}