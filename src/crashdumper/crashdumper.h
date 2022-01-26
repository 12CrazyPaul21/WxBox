#ifndef __WXBOX_CRASH_DUMPER_H
#define __WXBOX_CRASH_DUMPER_H

#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

typedef struct CrashExceptionInfo
{
    ::EXCEPTION_POINTERS exception;

    CrashExceptionInfo()
    {
        exception.ExceptionRecord = new ::EXCEPTION_RECORD;
        exception.ContextRecord   = new ::CONTEXT;

        ::memset(exception.ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));
        ::memset(exception.ContextRecord, 0, sizeof(CONTEXT));
    }

    ~CrashExceptionInfo()
    {
        if (exception.ContextRecord) {
            delete exception.ContextRecord;
        }

        ::EXCEPTION_RECORD* cursor = exception.ExceptionRecord;
        while (cursor) {
            auto next = cursor->ExceptionRecord;
            delete cursor;
            cursor = next;
        }
    }

    bool CollectionException(EXCEPTION_POINTERS* remoteExceptionPointers, HANDLE hProcess, HANDLE hThread)
    {
        if (!remoteExceptionPointers || !hProcess || !hThread) {
            return false;
        }

        if (!exception.ExceptionRecord || !exception.ContextRecord) {
            return false;
        }

        EXCEPTION_POINTERS remoteCrashProcessExceptionEntity = {0};

        // read record and context remote pointers from crash process
        SIZE_T szNumberOfBytesRead = 0;
        if (!ReadProcessMemory(hProcess, remoteExceptionPointers, &remoteCrashProcessExceptionEntity, sizeof(remoteCrashProcessExceptionEntity), &szNumberOfBytesRead)) {
            return false;
        }

        // read context
        if (!ReadProcessMemory(hProcess, remoteCrashProcessExceptionEntity.ContextRecord, exception.ContextRecord, sizeof(::CONTEXT), &szNumberOfBytesRead)) {
            return false;
        }

        //
        // deep copy exception record
        //

        // read exception record list header
        if (!ReadProcessMemory(hProcess, remoteCrashProcessExceptionEntity.ExceptionRecord, exception.ExceptionRecord, sizeof(::EXCEPTION_RECORD), &szNumberOfBytesRead)) {
            return false;
        }

        auto cursor     = exception.ExceptionRecord;
        auto nextRecord = cursor->ExceptionRecord;

        while (nextRecord) {
            auto newRecord = new EXCEPTION_RECORD;
            if (!ReadProcessMemory(hProcess, nextRecord, newRecord, sizeof(::EXCEPTION_RECORD), &szNumberOfBytesRead)) {
                return false;
            }

            cursor->ExceptionRecord = newRecord;
            cursor                  = newRecord;
            nextRecord              = cursor->ExceptionRecord;
        }

        return true;
    }

} CrashExceptionInfo, *PCrashExceptionInfo;

#endif

typedef struct CallStackInfo
{
    uint64_t    va;
    uint64_t    rva;
    std::string symbolName;

    uint64_t    moduleBaseAddress;
    uint64_t    moduleSize;
    std::string moduleName;
    std::string moduleImagePath;
} CallStackInfo, PCallStackInfo;

typedef struct CrashDumpReport
{
    std::string timestamp;
    std::string date;

    uint64_t pid;
    uint64_t tid;
    bool     is64Process;

    std::string systemVersionDescription;
    std::string cpuProductDescription;

    std::string crashProgram;
    std::string crashProgramFullPath;
    std::string crashProgramVersion;

    std::string crashThreadDescription;
    uint64_t    crashInstructionAddress;
    std::string crashSymbol;
    std::string crashModule;
    std::string exceptionTypeDescription;

    size_t                              maxLongCallStackSymbol;
    std::vector<CallStackInfo>          callStack;
    std::vector<wb_process::ModuleInfo> moduleInfos;

#if WXBOX_IN_WINDOWS_OS
    CONTEXT context;
#else

#endif

} CrashDumpReport, *PCrashDumpReport;

using CrashDumpReportPtr = std::unique_ptr<CrashDumpReport>;

#endif  // #ifndef __WXBOX_CRASH_DUMPER_H