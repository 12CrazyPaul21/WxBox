#include <utils/common.h>

//
// Global Variables
//

static std::mutex                         g_mutexHandleUnhandleException;
static std::atomic_bool                   g_alreadyRegistered     = false;
static wb_coredump::ExceptionExitCallback g_exceptionExitCallback = nullptr;

static std::string g_dumpPrefix   = "crashdump";
static std::string g_dumpSinkPath = "";
static std::string g_dumperPath   = "";

static std::string g_i18nPath = "";
static std::string g_language = "zh_cn";

static std::string g_themePath        = "";
static std::string g_currentThemeName = "";

//
// Functions
//

#if WXBOX_IN_WINDOWS_OS

#include <new.h>

static bool ReportExceptionToDumper(EXCEPTION_POINTERS* exception)
{
    static char shareName[MAX_PATH + 1] = {0};

    bool                             retval       = false;
    HANDLE                           hShared      = NULL;
    HANDLE                           hFinishEvent = NULL;
    PROCESS_INFORMATION              pi           = {0};
    STARTUPINFOA                     si           = {0};
    HANDLE                           hObjects[2]  = {0, 0};
    wb_coredump::CrashDumperRequest* report       = nullptr;

    if (!exception) {
        return retval;
    }

    // check dumper
    if (!wb_file::IsPathExists(g_dumperPath)) {
        return retval;
    }

    // generate crash timestamp
    auto timestamp     = wb_process::GetCurrentTimestamp(false);
    auto timestampDesc = wb_process::TimeStampToDate<std::chrono::seconds>(timestamp, false);

    // generate shared name
    sprintf_s(shareName, "__%s_crash_report_shared_%s__", g_dumpPrefix.c_str(), timestampDesc.c_str());

    // create file mapping
    hShared = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(wb_coredump::CrashDumperRequest), shareName);
    if (!hShared) {
        return retval;
    }

    // map memory
    report = (wb_coredump::CrashDumperRequest*)MapViewOfFile(hShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(wb_coredump::CrashDumperRequest));
    if (!report) {
        goto _FAILED;
    }

    //
    // fill crash dumper request
    //

    ::memset(report, 0, sizeof(wb_coredump::CrashDumperRequest));

    // record base information
    report->is64Process = wb_process::Is64Process(::GetCurrentProcess());
    report->pid         = ::GetCurrentProcessId();
    report->tid         = ::GetCurrentThreadId();
    report->exception   = exception;

    // record program information
    ::GetModuleBaseNameA(::GetCurrentProcess(), NULL, report->imageName, sizeof(report->imageName));
    strcpy_s(report->binRoot, wb_file::GetProcessRootPath().c_str());

    // record dump prefix and dump sink path
    strcpy_s(report->dumpPrefix, g_dumpPrefix.c_str());
    strcpy_s(report->dumpSinkPath, g_dumpSinkPath.c_str());

    // record i18n info
    strcpy_s(report->i18nPath, g_i18nPath.c_str());
    strcpy_s(report->language, g_language.c_str());

    // record theme info
    strcpy_s(report->themePath, g_themePath.c_str());
    strcpy_s(report->themeName, g_currentThemeName.c_str());

    // record crash timestamp
    strcpy_s(report->crashTimestamp, timestampDesc.c_str());
    strcpy_s(report->crashDate, wb_process::TimeStampToDate<std::chrono::seconds>(timestamp, true).c_str());

    // generate event name
    sprintf_s(report->finishEventName, "__%s_crash_dump_finish_event_%s__", g_dumpPrefix.c_str(), report->crashTimestamp);

    //
    // create finish event
    //

    hFinishEvent = ::CreateEventA(NULL, FALSE, FALSE, report->finishEventName);
    if (!hFinishEvent) {
        goto _FAILED;
    }

    //
    // unmap
    //

    UnmapViewOfFile(report);

    //
    // run dumper
    //

    if (!::CreateProcessA(g_dumperPath.c_str(), shareName, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
        goto _FAILED;
    }

    //
    // wait for finish
    //

    hObjects[0] = pi.hProcess;
    hObjects[1] = hFinishEvent;
    retval      = (::WaitForMultipleObjects(2, hObjects, FALSE, INFINITE) == WAIT_OBJECT_0 + 1);

_FAILED:

    CloseHandleSafe(pi.hThread);
    CloseHandleSafe(pi.hProcess);
    CloseHandleSafe(hFinishEvent);
    CloseHandleSafe(hShared);
    return retval;
}

static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exception)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);

    if (!exception) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //
    // report to dumper, execute minidump in dumper
    //

    LONG retval = EXCEPTION_EXECUTE_HANDLER;

    if (!ReportExceptionToDumper(exception)) {
        //
        // if failed, deal with myself
        //

        wb_coredump::GenerateMiniDumpParameter parameter;
        parameter.hProcess        = GetCurrentProcess();
        parameter.pid             = GetCurrentProcessId();
        parameter.tid             = GetCurrentThreadId();
        parameter.bClientPointers = TRUE;
        parameter.exception       = exception;
        parameter.dumpSinkPath    = g_dumpSinkPath;
        parameter.dumpPrefix      = g_dumpPrefix;
        parameter.timestamp       = wb_process::TimeStampToDate<std::chrono::seconds>(wb_process::GetCurrentTimestamp(false), false);

        if (!wb_coredump::GenerateMiniDump(parameter)) {
            retval = EXCEPTION_CONTINUE_SEARCH;
        }
    }

    if (g_exceptionExitCallback) {
        g_exceptionExitCallback();
    }

    return retval;
}

static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exception)
{
    return ExceptionHandler(exception);
}

static void WINAPI TriggerUnhandledException()
{
    __try {
        RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
    }
    __except (ExceptionHandler(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
    }
}

static void __cdecl InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
    UNREFERENCED_PARAMETER(expression);
    UNREFERENCED_PARAMETER(function);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(line);
    UNREFERENCED_PARAMETER(pReserved);

#if _DEBUG
    TriggerUnhandledException();
#endif
}

static void __cdecl ThreadLocalInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
    UNREFERENCED_PARAMETER(expression);
    UNREFERENCED_PARAMETER(function);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(line);
    UNREFERENCED_PARAMETER(pReserved);

#if _DEBUG
    TriggerUnhandledException();
#endif
}

static void __cdecl PurecallHandler()
{
#if _DEBUG
    TriggerUnhandledException();
#endif
}

static int __cdecl NewErrorHandler(size_t size)
{
    UNREFERENCED_PARAMETER(size);

#if _DEBUG
    TriggerUnhandledException();
#endif
    return 0;
}

static void _cdecl AbortSignalHandler(int sig)
{
    UNREFERENCED_PARAMETER(sig);

    // this is required, otherwise if there is another thread
    // simultaneously tries to abort process will be terminated
    signal(SIGABRT, AbortSignalHandler);

    TriggerUnhandledException();
}

static BOOL EnableCrashingOnCrashes()
{
    // ensure that the program crash request of the system is turned on
    // from article <When Even Crashing Doesn't Work>
    // link: https://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/

    static constexpr DWORD EXCEPTION_SWALLOWING = 0x1;

    using FnGetPolicy = BOOL(WINAPI*)(LPDWORD lpFlags);
    using FnSetPolicy = BOOL(WINAPI*)(DWORD dwFlags);

    HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
    if (!hKernel32) {
        return FALSE;
    }

    FnGetPolicy pfnGetPolicy = (FnGetPolicy)GetProcAddress(hKernel32, "GetProcessUserModeExceptionPolicy");
    FnSetPolicy pfnSetPolicy = (FnSetPolicy)GetProcAddress(hKernel32, "SetProcessUserModeExceptionPolicy");
    if (pfnGetPolicy && pfnSetPolicy) {
        DWORD dwFlags = 0;
        if (pfnGetPolicy(&dwFlags)) {
            // Turn off the filter
            return pfnSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING);
        }
    }

    return FALSE;
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilterDummy(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    UNREFERENCED_PARAMETER(lpTopLevelExceptionFilter);
    return NULL;
}

static BOOL PreventSetUnhandledExceptionFilter(bool prevent = true)
{
    HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
    if (!hKernel32) {
        return FALSE;
    }

    void* pfnSetUnhandledExceptionFilter = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
    if (!pfnSetUnhandledExceptionFilter) {
        return FALSE;
    }

    if (!prevent) {
        return wb_hook::RevokeInProcessHook(pfnSetUnhandledExceptionFilter);
    }

    return wb_hook::InProcessDummyHook(pfnSetUnhandledExceptionFilter, SetUnhandledExceptionFilterDummy);
}

static bool RegisterUnhandledExceptionAutoDumper_Windows(bool disabledWhenDebug)
{
    // don't set exception handler in debug mode
    if (IsDebuggerPresent() && disabledWhenDebug) {
        return false;
    }

    // prevent any error dialog
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    // disable any crt report message box
    _CrtSetReportMode(_CRT_WARN, 0);
    _CrtSetReportMode(_CRT_ERROR, 0);
    _CrtSetReportMode(_CRT_ASSERT, 0);

    // set unhandled exception handler
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);

    // set invalid parameter handler
    _set_invalid_parameter_handler(InvalidParameterHandler);

    // set thread local invalid parameter handler
    //_set_thread_local_invalid_parameter_handler(ThreadLocalInvalidParameterHandler);

    // set cpp purecall hanlder
    _set_purecall_handler(PurecallHandler);

    // set new error handler
    _set_new_handler(NewErrorHandler);

    // set abort handler
    signal(SIGABRT, AbortSignalHandler);
    _set_abort_behavior(0, 0);

    // ensure that the program crash request of the system is turned on
    EnableCrashingOnCrashes();

    // hook SetUnhandledExceptionFilter, avoid deleting our UnhandledExceptionHandler by CRT
    PreventSetUnhandledExceptionFilter();

    return true;
}

#elif WXBOX_IN_MAC_OS

static bool RegisterUnhandledExceptionAutoDumper_Mac(bool disabledWhenDebug)
{
    throw std::exception("RegisterUnhandledExceptionAutoDumper_Mac stub");
    return false;
}

#endif

bool wxbox::util::coredump::RegisterUnhandledExceptionAutoDumper(const std::string& dumpPrefix, const std::string& dumpSinkPath, const std::string& dumperPath, const std::string& i18nPath, const std::string& themePath, bool disabledWhenDebug)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);
    if (g_alreadyRegistered) {
        return false;
    }

#if WXBOX_IN_WINDOWS_OS
    g_alreadyRegistered = RegisterUnhandledExceptionAutoDumper_Windows(disabledWhenDebug);
#elif WXBOX_IN_MAC_OS
    g_alreadyRegistered = RegisterUnhandledExceptionAutoDumper_Mac(disabledWhenDebug);
#endif

    if (g_alreadyRegistered) {
        g_dumpPrefix   = wb_string::Utf8ToNativeString(dumpPrefix);
        g_dumpSinkPath = dumpSinkPath;
        g_dumperPath   = dumperPath;
        g_i18nPath     = i18nPath;
        g_themePath    = themePath;
    }

    return g_alreadyRegistered;
}

void wxbox::util::coredump::ChangeDumpePrefix(const std::string& prefix)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);
    g_dumpPrefix = wb_string::Utf8ToNativeString(prefix);
}

void wxbox::util::coredump::ChangeDumperLanguage(const std::string& language)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);
    g_language = language;
}

void wxbox::util::coredump::ChangeTheme(const std::string& themeName)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);
    g_currentThemeName = themeName;
}

void wxbox::util::coredump::RegisterExceptionExitCallback(wb_coredump::ExceptionExitCallback callback)
{
    std::lock_guard<std::mutex> lock(g_mutexHandleUnhandleException);
    g_exceptionExitCallback = callback;
}

//
// only for windows
//

#if WXBOX_IN_WINDOWS_OS

bool wxbox::util::coredump::GenerateMiniDump(const GenerateMiniDumpParameter& parameter)
{
    // generate coredump filename
    auto coredumpFileName = parameter.dumpPrefix + "-MiniDump-" + parameter.timestamp + ".dmp";

    // ensure sink path exists
    auto sinkPathExists = wb_file::IsPathExists(parameter.dumpSinkPath);
    if (sinkPathExists && !wb_file::IsDirectory(parameter.dumpSinkPath)) {
        return false;
    }
    if (!sinkPathExists) {
        // recursively create sink path
        if (!wb_file::RecursivelyCreateFolder(parameter.dumpSinkPath)) {
            return false;
        }
    }

    // join coredump filepath
    auto coredumpFilePath = wb_file::JoinPath(parameter.dumpSinkPath, coredumpFileName);

    // create file
    HANDLE hDumpFile = CreateFileA(coredumpFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDumpFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ThreadId          = parameter.tid;
    dumpInfo.ExceptionPointers = parameter.exception;
    dumpInfo.ClientPointers    = parameter.bClientPointers;

    bool result = MiniDumpWriteDump(parameter.hProcess, parameter.pid, hDumpFile, MiniDumpNormal, parameter.exception ? &dumpInfo : nullptr, nullptr, nullptr);
    CloseHandle(hDumpFile);
    return result;
}

#endif