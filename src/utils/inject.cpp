#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

BEGIN_NAKED_STD_FUNCTION(CallProcessModuleMethodStub, wb_inject::RemoteCallParameter* p)
{
    WXBOX_UNREF(p);

#if WXBOX_CPU_IS_X86
    __asm {
		push ebp
		mov ebp, esp
		push esi

        ;  // check whether the pointer is nullptr
		mov esi, [ebp+0x8]
		cmp esi, 0
		je _Ret

        ;  // check whether the parameter is valid
		cmp [esi]p.pModuleName, 0
		je _Ret
		cmp [esi]p.pFuncName, 0
		je _Ret
		cmp [esi]p.pFuncGetProcAddress, 0
		je _Ret
		cmp [esi]p.pFuncGetModuleHandleA, 0
		je _Ret

        ;  // call GetModuleHandleA
		push [esi]p.pModuleName
		call [esi]p.pFuncGetModuleHandleA
		cmp eax, 0
		je _Ret

        ;  // call GetProcAddress
		push [esi]p.pFuncName
		push eax
		call [esi]p.pFuncGetProcAddress
		cmp eax, 0
		je _Ret

        ;  // call target
		push [esi]p.pArg
		call eax

		;  // if it's cdecl, then actively clean stack
		cmp [esi]p.stdcallPromise, 0
		jne _NotStdcall
		add esp, 4

	_NotStdcall:
		mov eax, 0

	_Ret:
		pop esi
		pop ebp
		ret 4
    }
#endif  // #if WXBOX_CPU_IS_X86
}
END_NAKED_STD_FUNCTION(CallProcessModuleMethodStub);

#endif  // #if WXBOX_IN_WINDOWS_OS

static wb_traits::FunctionInfo GetCallProcessModuleMethodStubInfo()
{
#if WXBOX_IN_WINDOWS_OS
    return CallProcessModuleMethodStubInfo();
#else
    return wb_traits::FunctionInfo();
#endif
}

#if WXBOX_IN_WINDOWS_OS

static void AddModuleSearchPath_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, const std::string& moduleFolderPath)
{
    if (hProcess == ::GetCurrentProcess()) {
        SetDllDirectoryA(moduleFolderPath.c_str());
        return;
    }

    wb_inject::MethodCallingParameter parameter = wb_inject::MethodCallingParameter::BuildBufferValue(const_cast<char*>(moduleFolderPath.c_str()), moduleFolderPath.length() + 1);
    CallProcessModuleMethod(hProcess, "kernel32", "SetDllDirectoryA", &parameter, true);
}

static bool InjectModuleToProcess_Windows(wxbox::util::process::PID pid, const std::string& modulePath, const std::string& entryMethod, wb_inject::PMethodCallingParameter parameter, bool stdcallPromise)
{
    if (!pid || modulePath.empty()) {
        return false;
    }

    bool                               retval           = false;
    HMODULE                            hKernel32        = NULL;
    FARPROC                            funcLoadLibraryA = nullptr;
    HANDLE                             hRemoteThread    = NULL;
    wb_memory::RemotePageInfo          dataPageInfo;
    wb_memory::RemoteWrittenMemoryInfo modulePathMemoryInfo;

    // open process with all access permission
    HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return retval;
    }

    // allocate data page to process
    dataPageInfo = wb_memory::AllocPageToRemoteProcess(hProcess, wb_memory::RemotePageInfo::MIN_REMOTE_PAGE_SIZE * 2);
    if (!dataPageInfo.addr) {
        goto _DONE;
    }

    // write module path to process
    modulePathMemoryInfo = wb_memory::WriteStringToProcess(hProcess, dataPageInfo, modulePath);
    if (!modulePathMemoryInfo.addr) {
        goto _DONE;
    }

    // get kernel32 module handler
    hKernel32 = ::GetModuleHandleA("kernel32");
    if (!hKernel32) {
        goto _DONE;
    }

    // get LoadLibraryA address
    funcLoadLibraryA = ::GetProcAddress(hKernel32, "LoadLibraryA");
    if (!funcLoadLibraryA) {
        goto _DONE;
    }

    // inject dll module
    hRemoteThread = ::CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)funcLoadLibraryA, modulePathMemoryInfo.addr, 0, nullptr);
    if (!hRemoteThread) {
        goto _DONE;
    }

    // wait for module loaded
    ::WaitForSingleObject(hRemoteThread, INFINITE);

    // inject success
    retval = true;

    // invoke entry if exist
    if (entryMethod.empty()) {
        goto _DONE;
    }
    retval = wb_inject::CallProcessModuleMethod(hProcess, dataPageInfo, wb_file::ToFileName(modulePath), entryMethod, parameter, stdcallPromise);

_DONE:
    wb_memory::FreeRemoteProcessPage(hProcess, dataPageInfo);
    CloseHandleSafe(hRemoteThread);
    wb_process::CloseProcessHandle(hProcess);
    return retval;
}

static bool UnInjectModuleFromProcess_Windows(wxbox::util::process::PID pid, const std::string& moduleName)
{
    if (!pid) {
        return false;
    }

    bool                   retval          = false;
    HMODULE                hKernel32       = NULL;
    FARPROC                funcFreeLibrary = nullptr;
    HANDLE                 hRemoteThread   = NULL;
    wb_process::ModuleInfo remoteModuleInfo;

    // get forhook.dll module handle
    if (!wb_process::GetModuleInfo(pid, moduleName, remoteModuleInfo)) {
        return retval;
    }

    // current process
    if (pid == ::GetCurrentProcessId()) {
        ::FreeLibrary(remoteModuleInfo.hModule);
        return true;
    }

    // open process with all access permission
    HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return retval;
    }

    // get kernel32 module handler
    hKernel32 = ::GetModuleHandleA("kernel32");
    if (!hKernel32) {
        goto _DONE;
    }

    // get FreeLibrary address
    funcFreeLibrary = ::GetProcAddress(hKernel32, "FreeLibrary");
    if (!funcFreeLibrary) {
        goto _DONE;
    }

    // inject dll module
    hRemoteThread = ::CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)funcFreeLibrary, remoteModuleInfo.hModule, 0, nullptr);
    if (!hRemoteThread) {
        goto _DONE;
    }

    // wait for module loaded
    ::WaitForSingleObject(hRemoteThread, INFINITE);

    // inject success
    retval = true;

_DONE:
    CloseHandleSafe(hRemoteThread);
    wb_process::CloseProcessHandle(hProcess);
    return retval;
}

typedef struct _UnloadModuleBySelfParameter
{
    wb_process::MODULE_HANDLE hModule;
    wb_process::TID           tid;
} UnloadModuleBySelfParameter, *PUnloadModuleBySelfParameter;

static DWORD WINAPI UnloadModuleBySelf_ThreadProc(LPVOID lpParam)
{
    PUnloadModuleBySelfParameter parameter = reinterpret_cast<PUnloadModuleBySelfParameter>(lpParam);
    if (!parameter) {
        return 0;
    }

    HMODULE         hModule = parameter->hModule;
    wb_process::TID tid     = parameter->tid;
    delete parameter;

    // wait for target thread finish
    if (tid) {
        HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
    }

    // unload library
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

static bool UnloadModuleBySelf_Windows(const std::string& moduleName, wb_process::TID triggerThreadId)
{
    PUnloadModuleBySelfParameter parameter = new UnloadModuleBySelfParameter();
    if (!parameter) {
        return false;
    }

    if (!::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, moduleName.c_str(), &parameter->hModule)) {
        return false;
    }
    parameter->tid = triggerThreadId;

    return ::CreateThread(NULL, 0, UnloadModuleBySelf_ThreadProc, parameter, 0, nullptr);
}

static bool CallProcessModuleMethod_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, wb_inject::PMethodCallingParameter parameter, bool stdcallPromise)
{
    if (!hProcess || moduleName.empty() || method.empty()) {
        return false;
    }

    HMODULE                            hKernel32            = NULL;
    FARPROC                            funcGetModuleHandleA = nullptr;
    decltype(funcGetModuleHandleA)     funcGetProcAddress   = nullptr;
    HANDLE                             hRemoteThread        = NULL;
    wb_inject::RemoteCallParameter     remoteCallParameter;
    wb_traits::FunctionInfo            funcCallProcessModuleMethodStubInfo;
    wb_memory::RemoteWrittenMemoryInfo moduleNameMemoryInfo;
    decltype(moduleNameMemoryInfo)     methodNameMemoryInfo;
    decltype(moduleNameMemoryInfo)     remoteCallParameterMemoryInfo;
    decltype(moduleNameMemoryInfo)     remoteCodeStreamMemoryInfo;

    // get kernel32 module handler
    hKernel32 = ::GetModuleHandleA("kernel32");
    if (!hKernel32) {
        return false;
    }

    // get GetModuleHandleA address
    funcGetModuleHandleA = ::GetProcAddress(hKernel32, "GetModuleHandleA");
    if (!funcGetModuleHandleA) {
        return false;
    }

    // get GetProcAddress address
    funcGetProcAddress = ::GetProcAddress(hKernel32, "GetProcAddress");
    if (!funcGetProcAddress) {
        return false;
    }

    // write string info to process
    moduleNameMemoryInfo = wb_memory::WriteStringToProcess(hProcess, dataPageInfo, moduleName);
    methodNameMemoryInfo = wb_memory::WriteStringToProcess(hProcess, dataPageInfo, method);
    if (!moduleNameMemoryInfo.addr || !methodNameMemoryInfo.addr) {
        return false;
    }

    // packaging RemoteCallParameter
    remoteCallParameter.pModuleName           = (char*)moduleNameMemoryInfo.addr;
    remoteCallParameter.pFuncName             = (char*)methodNameMemoryInfo.addr;
    remoteCallParameter.pArg                  = 0;
    remoteCallParameter.pFuncGetModuleHandleA = funcGetModuleHandleA;
    remoteCallParameter.pFuncGetProcAddress   = funcGetProcAddress;
    remoteCallParameter.stdcallPromise        = stdcallPromise ? 1 : 0;

    // handle method calling parameter
    if (parameter) {
        switch (parameter->type) {
            case wb_inject::MethodCallingParameterType::CpuWordLongScalarValue:
                remoteCallParameter.pArg = (void*)parameter->value;
                break;
            case wb_inject::MethodCallingParameterType::BufferPointer: {
                wb_memory::RemoteWrittenMemoryInfo parameterMemoryInfo = wb_memory::WriteByteStreamToProcess(hProcess, dataPageInfo, reinterpret_cast<const uint8_t* const>(parameter->value), parameter->size);
                if (!parameterMemoryInfo.addr) {
                    // maybe space not enough
                    return false;
                }
                remoteCallParameter.pArg = parameterMemoryInfo.addr;
                break;
            }
        }
    }

    // write RemoteCallParameter to process
    remoteCallParameterMemoryInfo = wb_memory::WriteByteStreamToProcess(hProcess, dataPageInfo, reinterpret_cast<const uint8_t* const>(&remoteCallParameter), sizeof(remoteCallParameter));
    if (!remoteCallParameterMemoryInfo.addr) {
        return false;
    }

    // get CallProcessModuleMethodStub info
    funcCallProcessModuleMethodStubInfo = GetCallProcessModuleMethodStubInfo();
    if (!funcCallProcessModuleMethodStubInfo.addr) {
        return false;
    }

    // write code to process
    remoteCodeStreamMemoryInfo = wb_memory::WriteByteStreamToProcess(hProcess, codePageInfo, reinterpret_cast<const uint8_t* const>(funcCallProcessModuleMethodStubInfo.addr), funcCallProcessModuleMethodStubInfo.size);
    if (!remoteCodeStreamMemoryInfo.addr) {
        return false;
    }

    // invoke
    hRemoteThread = ::CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)remoteCodeStreamMemoryInfo.addr, remoteCallParameterMemoryInfo.addr, 0, nullptr);
    if (!hRemoteThread) {
        return false;
    }

    // wait for module loaded
    ::WaitForSingleObject(hRemoteThread, INFINITE);

    CloseHandleSafe(hRemoteThread);
    return true;
}

#else

static void AddModuleSearchPath_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const std::string& moduleFolderPath)
{
    throw std::exception("AddModuleSearchPath_Mac stub");
}

static bool InjectModuleToProcess_Mac(wxbox::util::process::PID pid, const std::string& modulePath, const std::string& entryMethod, wb_inject::PMethodCallingParameter parameter, bool stdcallPromise)
{
    throw std::exception("InjectModuleToProcess_Mac stub");
    return false;
}

static bool UnInjectModuleFromProcess_Mac(wxbox::util::process::PID pid, const std::string& moduleName)
{
    throw std::exception("UnInjectModuleFromProcess_Mac stub");
    return false;
}

static bool UnloadModuleBySelf_Mac(const std::string& moduleName, wb_process::TID triggerThreadId)
{
    throw std::exception("UnloadModuleBySelf_Mac stub");
    return false;
}

static bool CallProcessModuleMethod_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, wb_inject::PMethodCallingParameter parameter, bool stdcallPromise)
{
    throw std::exception("CallProcessModuleMethod_Mac stub");
    return false;
}

#endif

void wxbox::util::inject::AddModuleSearchPath(wxbox::util::process::PROCESS_HANDLE hProcess, const std::string& moduleFolderPath)
{
    if (!hProcess || moduleFolderPath.empty()) {
        return;
    }

#if WXBOX_IN_WINDOWS_OS
    return AddModuleSearchPath_Windows(hProcess, moduleFolderPath);
#else
    return AddModuleSearchPath_Mac(hProcess, moduleFolderPath);
#endif
}

void wxbox::util::inject::AddModuleSearchPath(wxbox::util::process::PID pid, const std::string& moduleFolderPath)
{
    wb_process::PROCESS_HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return;
    }
    AddModuleSearchPath(hProcess, moduleFolderPath);
    wb_process::CloseProcessHandle(hProcess);
}

bool wxbox::util::inject::InjectModuleToProcess(wxbox::util::process::PID pid, const std::string& modulePath, const std::string& entryMethod, PMethodCallingParameter parameter, bool stdcallPromise)
{
#if WXBOX_IN_WINDOWS_OS
    return InjectModuleToProcess_Windows(pid, modulePath, entryMethod, parameter, stdcallPromise);
#else
    return InjectModuleToProcess_Mac(pid, modulePath, entryMethod, parameter, stdcallPromise);
#endif
}

bool wxbox::util::inject::UnInjectModuleFromProcess(wxbox::util::process::PID pid, const std::string& moduleName)
{
#if WXBOX_IN_WINDOWS_OS
    return UnInjectModuleFromProcess_Windows(pid, moduleName);
#else
    return UnInjectModuleFromProcess_Mac(pid, moduleName);
#endif
}

bool wxbox::util::inject::UnloadModuleBySelf(const std::string& moduleName, wb_process::TID triggerThreadId)
{
#if WXBOX_IN_WINDOWS_OS
    return UnloadModuleBySelf_Windows(moduleName, triggerThreadId);
#else
    return UnloadModuleBySelf_Mac(moduleName, triggerThreadId);
#endif
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
#if WXBOX_IN_WINDOWS_OS
    return CallProcessModuleMethod_Windows(hProcess, dataPageInfo, codePageInfo, moduleName, method, parameter, stdcallPromise);
#else
    return CallProcessModuleMethod_Mac(hProcess, dataPageInfo, codePageInfo, moduleName, method, parameter, stdcallPromise);
#endif
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PID pid, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
    bool                       retval   = false;
    wb_process::PROCESS_HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return false;
    }
    retval = CallProcessModuleMethod(hProcess, dataPageInfo, codePageInfo, moduleName, method, parameter, stdcallPromise);
    wb_process::CloseProcessHandle(hProcess);
    return retval;
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
    if (!hProcess) {
        return false;
    }

    wb_traits::FunctionInfo funcCallProcessModuleMethodStubInfo = GetCallProcessModuleMethodStubInfo();
    if (!funcCallProcessModuleMethodStubInfo.addr) {
        return false;
    }

    bool retval = false;

    wb_memory::RemotePageInfo codePageInfo = wb_memory::AllocPageToRemoteProcess(hProcess, funcCallProcessModuleMethodStubInfo.size, true);
    if (!codePageInfo.addr) {
        return false;
    }

    retval = CallProcessModuleMethod(hProcess, dataPageInfo, codePageInfo, moduleName, method, parameter, stdcallPromise);
    wb_memory::FreeRemoteProcessPage(hProcess, codePageInfo);
    return retval;
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PID pid, wxbox::util::memory::RemotePageInfo& dataPageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
    bool                       retval   = false;
    wb_process::PROCESS_HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return false;
    }
    retval = CallProcessModuleMethod(hProcess, dataPageInfo, moduleName, method, parameter, stdcallPromise);
    wb_process::CloseProcessHandle(hProcess);
    return retval;
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
    if (!hProcess) {
        return false;
    }

    bool                      retval       = false;
    wb_memory::RemotePageInfo dataPageInfo = wb_memory::AllocPageToRemoteProcess(hProcess);
    if (!dataPageInfo.addr) {
        return false;
    }

    retval = CallProcessModuleMethod(hProcess, dataPageInfo, moduleName, method, parameter, stdcallPromise);
    wb_memory::FreeRemoteProcessPage(hProcess, dataPageInfo);
    return retval;
}

bool wxbox::util::inject::CallProcessModuleMethod(wxbox::util::process::PID pid, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter, bool stdcallPromise)
{
    bool                       retval   = false;
    wb_process::PROCESS_HANDLE hProcess = wb_process::OpenProcessHandle(pid);
    if (!hProcess) {
        return false;
    }
    retval = CallProcessModuleMethod(hProcess, moduleName, method, parameter, stdcallPromise);
    wb_process::CloseProcessHandle(hProcess);
    return retval;
}