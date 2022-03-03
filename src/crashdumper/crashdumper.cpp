#include <QApplication>
#include <crashdumper_dialog.h>

#undef signals
#include <crashdumper.h>
#define signals Q_SIGNALS

#if WXBOX_IN_WINDOWS_OS

static CrashDumpReportPtr AnalysisCrashDumpReport(wb_coredump::PCrashDumperRequest report, PCrashExceptionInfo crashExceptionInfo, HANDLE hProcess, HANDLE hThread)
{
    if (!report || !crashExceptionInfo || !crashExceptionInfo->exception.ExceptionRecord || !crashExceptionInfo->exception.ContextRecord || !hProcess || !hThread) {
        return nullptr;
    }

    if (!SymInitialize(hProcess, "", TRUE)) {
        return nullptr;
    }

    CrashDumpReportPtr dumpReport = std::make_unique<CrashDumpReport>();
    if (!dumpReport) {
        SymCleanup(hProcess);
        return nullptr;
    }

    STACKFRAME64      stackFrame = {0};
    IMAGEHLP_MODULE64 moduleInfo = {sizeof(IMAGEHLP_MODULE64)};
    CONTEXT           context    = *crashExceptionInfo->exception.ContextRecord;

    SYMBOL_INFO* symbolInfo = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + (MAX_PATH + 1) * sizeof(char), 1);
    if (!symbolInfo) {
        SymCleanup(hProcess);
        return nullptr;
    }
    symbolInfo->MaxNameLen   = MAX_PATH;
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);

    //
    // record base information
    //

    dumpReport->timestamp = report->crashTimestamp;
    dumpReport->date      = report->crashDate;

    dumpReport->pid         = report->pid;
    dumpReport->tid         = report->tid;
    dumpReport->context     = *crashExceptionInfo->exception.ContextRecord;
    dumpReport->is64Process = report->is64Process;

    dumpReport->systemVersionDescription = wb_platform::GetSystemVersionDescription();
    dumpReport->cpuProductDescription    = wb_platform::GetCPUProductBrandDescription();
    dumpReport->crashProgram             = report->imageName;
    dumpReport->crashProgramFullPath     = wb_file::JoinPath(report->binRoot, report->imageName);
    dumpReport->crashProgramVersion      = wb_file::GetFileVersion(dumpReport->crashProgramFullPath);

    //
    // record exception information
    //

    dumpReport->exceptionTypeDescription = wb_platform::ExceptionDescription(crashExceptionInfo->exception.ExceptionRecord->ExceptionCode);
#if !defined(_AMD64_)
    dumpReport->crashInstructionAddress = crashExceptionInfo->exception.ContextRecord->Eip;
#else
    dumpReport->crashInstructionAddress = crashExceptionInfo->exception.ContextRecord->Rip;
#endif

    // get crash thread description
    dumpReport->crashThreadDescription = wb_process::GetThreadName(hThread);

    if (dumpReport->crashInstructionAddress) {
        // get crash symbol
        if (SymFromAddr(hProcess, dumpReport->crashInstructionAddress, nullptr, symbolInfo)) {
            dumpReport->crashSymbol = symbolInfo->Name;
        }

        // get crash module
        if (SymGetModuleInfo64(hProcess, dumpReport->crashInstructionAddress, &moduleInfo)) {
            dumpReport->crashModule = moduleInfo.ImageName;
        }
    }

    //
    // record call stack information
    //

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

    dumpReport->maxLongCallStackSymbol = 0;
    while (StackWalk64(machineType, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        CallStackInfo callStackInfo;

        callStackInfo.va                = stackFrame.AddrPC.Offset;
        callStackInfo.rva               = 0;
        callStackInfo.moduleBaseAddress = 0;
        callStackInfo.moduleSize        = 0;

        // record symbol name(if exists)
        if (SymFromAddr(hProcess, stackFrame.AddrPC.Offset, nullptr, symbolInfo)) {
            callStackInfo.symbolName = symbolInfo->Name;
            if (callStackInfo.symbolName.length() > dumpReport->maxLongCallStackSymbol) {
                dumpReport->maxLongCallStackSymbol = callStackInfo.symbolName.length();
            }
        }

        if (SymGetModuleInfo64(hProcess, stackFrame.AddrPC.Offset, &moduleInfo)) {
            callStackInfo.moduleName        = moduleInfo.ModuleName;
            callStackInfo.moduleImagePath   = moduleInfo.ImageName;
            callStackInfo.moduleBaseAddress = moduleInfo.BaseOfImage;
            callStackInfo.moduleSize        = moduleInfo.ImageSize;
            callStackInfo.rva               = callStackInfo.va - callStackInfo.moduleBaseAddress;
        }

        dumpReport->callStack.emplace_back(std::move(callStackInfo));
    }

    // collect module info
    dumpReport->moduleInfos = wb_process::CollectModuleInfos(wb_process::PID(report->pid));

    free(symbolInfo);
    SymCleanup(hProcess);
    return dumpReport;
}

static std::string GenerateCrashDumpReport(PCrashDumpReport report)
{
    std::stringstream ss;

    if (!report) {
        return "";
    }

#define PRINT_HEX(value) \
    "0x" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(report->is64Process ? 16 : 8) << value

    //
    // print system version description
    //

    ss << "----------------------------------------------------" << std::endl;
    ss << " Environment Information : " << std::endl;
    ss << "----------------------------------------------------" << std::endl;
    ss << "System                    : " << report->systemVersionDescription << std::endl;
    ss << "CPU                       : " << report->cpuProductDescription << std::endl;
    ss << "Program Name              : " << report->crashProgram << std::endl;
    ss << "Program Path              : " << report->crashProgramFullPath << std::endl;
    if (!report->crashProgramVersion.empty()) {
        ss << "Program Version           : " << report->crashProgramVersion << std::endl;
    }
    ss << std::endl
       << std::endl;

    //
    // print base exception information
    //

    ss << "----------------------------------------------------" << std::endl;
    ss << " Exception Information : " << std::endl;
    ss << "----------------------------------------------------" << std::endl;
    ss << "Exception timestamp       : " << report->date << std::endl;
    ss << "Crash Process Image Name  : " << report->crashProgram << "(" << (report->is64Process ? "64" : "32") << "-bits)" << std::endl;
    ss << "Crash Process ID          : " << PRINT_HEX(report->pid) << std::endl;
    ss << "Crash Thread  ID          : " << PRINT_HEX(report->tid) << std::endl;
    if (!report->crashThreadDescription.empty()) {
        ss << "Crash Thread  Name        : " << report->crashThreadDescription << std::endl;
    }
    ss << "Exception Type            : " << report->exceptionTypeDescription << std::endl;
    ss << "Error Instruction Address : " << PRINT_HEX(report->crashInstructionAddress) << std::endl;
    if (!report->crashSymbol.empty()) {
        ss << "Crash Symbol              : " << report->crashSymbol << std::endl;
    }
    if (!report->crashModule.empty()) {
        ss << "Crash Module              : " << report->crashModule << std::endl;
    }
    ss << std::endl
       << std::endl;

    //
    // print call stack
    //

    ss << "----------------------------------------------------" << std::endl;
    ss << " Call Stack : " << std::endl;
    ss << "----------------------------------------------------" << std::endl;

    auto symbolAlign = report->maxLongCallStackSymbol + 6;
    for (auto callInfo : report->callStack) {
        auto symbol = "[" + (callInfo.symbolName.empty() ? " " : callInfo.symbolName) + "]";
        ss << std::left << std::setfill(' ') << std::setw(symbolAlign) << symbol;
        ss << callInfo.moduleName << "+" << PRINT_HEX(callInfo.rva);
        ss << std::endl;
    }
    ss << std::endl
       << std::endl;

    //
    // print thread context
    //

    ss << "----------------------------------------------------" << std::endl;
    ss << " Thread Context : " << std::endl;
    ss << "----------------------------------------------------" << std::endl;

    auto& context = report->context;
#if !defined(_AMD64_)
    ss << "EIP=" << PRINT_HEX(context.Eip) << std::endl;
    ss << "EAX=" << PRINT_HEX(context.Eax) << " EBX=" << PRINT_HEX(context.Ebx)
       << " ECX=" << PRINT_HEX(context.Ecx) << " EDX=" << PRINT_HEX(context.Edx) << std::endl;
    ss << "EBP=" << PRINT_HEX(context.Ebp) << " ESP=" << PRINT_HEX(context.Esp)
       << " ESI=" << PRINT_HEX(context.Esi) << " EDI=" << PRINT_HEX(context.Edi) << std::endl;
#else
    ss << "RIP=" << PRINT_HEX(context.Rip) << std::endl;
    ss << "RAX=" << PRINT_HEX(context.Rax) << " RBX=" << PRINT_HEX(context.Rbx)
       << " RCX=" << PRINT_HEX(context.Rcx) << " RDX=" << PRINT_HEX(context.Rdx) << std::endl;
    ss << "RBP=" << PRINT_HEX(context.Rbp) << " RSP=" << PRINT_HEX(context.Rsp)
       << " RSI=" << PRINT_HEX(context.Rsi) << " RDI=" << PRINT_HEX(context.Rdi) << std::endl;
    ss << "R8=" << PRINT_HEX(context.R8) << " R9=" << PRINT_HEX(context.R9)
       << " R10=" << PRINT_HEX(context.R10) << " R11=" << PRINT_HEX(context.R11) << std::endl;
    ss << "R12 =" << PRINT_HEX(context.R12) << " R13=" << PRINT_HEX(context.R13)
       << " R14=" << PRINT_HEX(context.R14) << " R=15" << PRINT_HEX(context.R15) << std::endl;
#endif
    ss << std::endl
       << std::endl;

    //
    // print thread context
    //

    ss << "----------------------------------------------------" << std::endl;
    ss << " Module Informations : " << std::endl;
    ss << "----------------------------------------------------" << std::endl;

    size_t maxLongModulePathName = 0;
    for (auto moduleInfo : report->moduleInfos) {
        if (moduleInfo.modulePath.length() > maxLongModulePathName) {
            maxLongModulePathName = moduleInfo.modulePath.length();
        }
    }

    auto modulePathAlign = maxLongModulePathName + 2;
    for (auto moduleInfo : report->moduleInfos) {
        ss << "[" << PRINT_HEX(moduleInfo.pModuleBaseAddr) << " - " << PRINT_HEX(((unsigned long long)moduleInfo.pModuleBaseAddr + moduleInfo.uModuleSize)) << "] ";
        ss << std::left << std::setfill(' ') << std::setw(modulePathAlign) << moduleInfo.modulePath;
        ss << "[size:" << PRINT_HEX(moduleInfo.uModuleSize) << "]";

        auto moduleVersion = wb_file::GetFileVersion(moduleInfo.modulePath);
        if (!moduleVersion.empty()) {
            ss << "  [v" << moduleVersion << "]";
        }
        ss << std::endl;
    }

    return ss.str();
}

static void GenerateAndSaveCrashReport(wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport)
{
    if (!request || !dumpReport) {
        return;
    }

    auto report = GenerateCrashDumpReport(dumpReport);
    if (!report.empty()) {
        char reportFileName[MAX_PATH + 1] = {0};
        sprintf_s(reportFileName, "%s-CrashReport-%s.txt", request->dumpPrefix, request->crashTimestamp);
        auto reportFilePath = wb_file::JoinPath(request->dumpSinkPath, reportFileName);

        std::ofstream stream(reportFilePath, std::ios::out | std::ios::trunc);
        if (stream.is_open()) {
            stream << report;
            stream.flush();
            stream.close();
        }
    }
}

static void ShowCrashReport(int argc, char* argv[], wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport)
{
    if (!request || !dumpReport) {
        return;
    }

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    xstyle_manager.RegisterI18nFolder(wb_string::NativeToUtf8String(request->i18nPath).c_str());
    xstyle_manager.RegisterThemeFolder(request->themePath);
    xstyle_manager.RegisterDefaultTheme(XSTYLE_DEFAULT_THEME_URL);
    xstyle_manager.ChangeLanguage(request->language);
    xstyle_manager.ChangeTheme(request->themeName);

    CrashReportDialog dialog(nullptr, request, dumpReport);
    dialog.show();
    app.exec();
}

static int main_Windows(int argc, char* argv[])
{
    if (argc < 1) {
        return 0;
    }

    wb_platform::EnableDebugPrivilege();

    // fetch shared name
    const char* const sharedName = argv[0];
    if (!sharedName || !strlen(sharedName)) {
        return 0;
    }

    // open file mapping
    HANDLE hShared = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedName);
    if (!hShared) {
        return 0;
    }

    // map CrashDumperRequest
    wb_coredump::CrashDumperRequest* pSharedReport = (wb_coredump::CrashDumperRequest*)MapViewOfFile(hShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(wb_coredump::CrashDumperRequest));
    if (!pSharedReport) {
        CloseHandle(hShared);
        return 0;
    }

    // copy report
    wb_coredump::CrashDumperRequest report = *pSharedReport;

    // close file mapping
    UnmapViewOfFile(pSharedReport);
    CloseHandle(hShared);

    // check if the architecture is the same as the target
    if (report.is64Process != wb_process::Is64Process(::GetCurrentProcess())) {
        return 0;
    }

    // verify crash dump parameter
    if (!report.pid || !report.tid || !report.exception) {
        return 0;
    }
    report.imageName[MAX_PATH]       = 0;
    report.binRoot[MAX_PATH]         = 0;
    report.dumpPrefix[MAX_PATH]      = 0;
    report.dumpSinkPath[MAX_PATH]    = 0;
    report.crashTimestamp[MAX_PATH]  = 0;
    report.finishEventName[MAX_PATH] = 0;

    // open crash process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DWORD(report.pid));
    if (!hProcess) {
        return 0;
    }

    // open crash thread
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, DWORD(report.tid));
    if (!hThread) {
        ::CloseHandle(hProcess);
        return 0;
    }

    // suspend all threads in crash process
    wb_process::SuspendAllOtherThread(wb_process::PID(report.pid), 0);

    // generate mindump
    wb_coredump::GenerateMiniDumpParameter parameter;
    parameter.hProcess        = hProcess;
    parameter.pid             = wb_process::PID(report.pid);
    parameter.tid             = wb_process::TID(report.tid);
    parameter.bClientPointers = TRUE;
    parameter.exception       = report.exception;
    parameter.dumpSinkPath    = report.dumpSinkPath;
    parameter.dumpPrefix      = report.dumpPrefix;
    parameter.timestamp       = report.crashTimestamp;
    bool miniDumpSuccess      = wb_coredump::GenerateMiniDump(parameter);

    // deep copy exception info and analysis crash dump report
    CrashExceptionInfo crashExceptionInfo;
    CrashDumpReportPtr dumpReport = nullptr;
    if (crashExceptionInfo.CollectionException(report.exception, hProcess, hThread)) {
        dumpReport = AnalysisCrashDumpReport(&report, &crashExceptionInfo, hProcess, hThread);
    }

    // resume crash thread
    ::ResumeThread(hThread);
    ::CloseHandle(hThread);

    // report dump success
    if (miniDumpSuccess) {
        HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, report.finishEventName);
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
    }

    if (!dumpReport || !miniDumpSuccess) {
        ::CloseHandle(hProcess);
        return 0;
    }

    // wait for crash process quit
    WaitForSingleObject(hProcess, INFINITE);
    ::CloseHandle(hProcess);

    // generate and show crash report
    GenerateAndSaveCrashReport(&report, dumpReport.get());
    ShowCrashReport(argc, argv, &report, dumpReport.get());

    return 0;
}

#else

static int main_Mac(int argc, char* argv[])
{
    throw std::exception("crashdumper main_Mac stub");
    return 0;
}

#endif

int main(int argc, char* argv[])
{
    wb_process::AppSingleton singleton("____WxBox_Crash_Dumper_Singleton_Mutex____", true);

#if WXBOX_IN_WINDOWS_OS
    return main_Windows(argc, argv);
#else
    return main_Mac(argc, argv);
#endif
}
