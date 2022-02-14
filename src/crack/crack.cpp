#include <utils/common.h>
#include <frida-gum.h>

#if WXBOX_IN_WINDOWS_OS

//
// Macro
//

#define DEBUG_MODE_WAITING_INTERVAL_MS 10000

//
// Function
//

static inline bool OpenWxWithMultiBoxing_CheckWeChatWinModule(HANDLE hProcess, const LOAD_DLL_DEBUG_INFO& loadDllDebugInfo, LPDWORD pdwModuleSize = nullptr)
{
    if (!hProcess || !loadDllDebugInfo.lpImageName) {
        return false;
    }

    BOOL    status                         = FALSE;
    SIZE_T  nNumberOfBytesRead             = 0;
    LPVOID  lpImageName                    = nullptr;
    uint8_t dllAbsPathBuffer[MAX_PATH * 2] = {0};

    // read image full path actual address
    status = ::ReadProcessMemory(hProcess, loadDllDebugInfo.lpImageName, &lpImageName, sizeof(lpImageName), &nNumberOfBytesRead);
    if (!status || !nNumberOfBytesRead || !lpImageName) {
        return false;
    }

    //  read image full path
    status = ::ReadProcessMemory(hProcess, lpImageName, (LPVOID)dllAbsPathBuffer, sizeof(dllAbsPathBuffer), &nNumberOfBytesRead);
    if (!status || !nNumberOfBytesRead) {
        return false;
    }

    // to string
    std::string moduleAbsPath;
    if (loadDllDebugInfo.fUnicode) {
        moduleAbsPath = wb_string::ToString((wchar_t*)dllAbsPathBuffer);
    }
    else {
        moduleAbsPath = (char*)dllAbsPathBuffer;
    }

    // get module file name
    std::string moduleFileName = wb_file::ToFileName(moduleAbsPath);

    // check for WeChatWin.dll
    if (::_stricmp(moduleFileName.c_str(), WX_WE_CHAT_CORE_MODULE)) {
        return false;
    }

    // get size of module
    *pdwModuleSize = wb_platform::GetPEModuleImageSize(moduleAbsPath);

    return true;
}

/**
 *
 * The instructions after crack are as follows:
 *
 *     void _declspec(naked) CheckAppSingleton_Crack()
 *     {
 *         __asm {
 *             mov eax, 0
 *             ret
 *         }
 *     }
 */
static inline bool OpenWxWithMultiBoxing_Crack(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wxbox::crack::feature::WxApiFeatures& wxApiFeatures, HANDLE hProcess, LPVOID pBaseAddr, DWORD dwModuleSize)
{
    if (!hProcess) {
        return false;
    }

    ucpulong_t checkAppSingletonVA = wxApiFeatures.Locate({hProcess, pBaseAddr, (ucpulong_t)dwModuleSize}, wxEnvInfo.version, "CheckAppSingleton");
    if (!checkAppSingletonVA) {
        return false;
    }

    //
    // asm 'ret' code. [0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3](mov eax, 0; ret) can be used directly, but frida-gum's GumX86Writer is still used for assembly here
    //

    GumX86Writer x86Writer;
    guint8       x86MachineInstruction[32] = {0};
    guint        uInstructionSize          = 0;

#if WXBOX_CPU_IS_X86
    ::gum_x86_writer_init(&x86Writer, x86MachineInstruction);
    ::gum_x86_writer_set_target_cpu(&x86Writer, GUM_CPU_IA32);
    ::gum_x86_writer_set_target_abi(&x86Writer, GUM_ABI_WINDOWS);
    ::gum_x86_writer_put_mov_reg_u32(&x86Writer, GUM_REG_EAX, 0);
    ::gum_x86_writer_put_ret(&x86Writer);
    uInstructionSize = ::gum_x86_writer_offset(&x86Writer);
#endif

    //
    // crack wechat
    //

    return wb_memory::WriteMemory(hProcess, (void*)checkAppSingletonVA, x86MachineInstruction, uInstructionSize, nullptr);
}

static inline bool OpenWxWithMultiBoxing_DebugLoop(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_process::PID pid, wb_crack::POpenWxWithMultiBoxingResult pResult)
{
    if (!pid) {
        return false;
    }

    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        return false;
    }

    bool        result       = false;
    DEBUG_EVENT debugEvent   = {0};
    DWORD       dwModuleSize = 0;

    for (;;) {
        if (!::WaitForDebugEventEx(&debugEvent, DEBUG_MODE_WAITING_INTERVAL_MS)) {
            break;
        }

        //
        // only handle "dll onload event" in target process
        //

        if (debugEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && debugEvent.dwProcessId == pid) {
            if (OpenWxWithMultiBoxing_CheckWeChatWinModule(hProcess, debugEvent.u.LoadDll, &dwModuleSize)) {
                result = OpenWxWithMultiBoxing_Crack(wxEnvInfo, wxApiFeatures, hProcess, debugEvent.u.LoadDll.lpBaseOfDll, dwModuleSize);
                if (pResult) {
                    pResult->pid             = pid;
                    pResult->pModuleBaseAddr = debugEvent.u.LoadDll.lpBaseOfDll;
                    pResult->uModuleSize     = dwModuleSize;
                }
                break;
            }
        }

        if (!::ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE)) {
            break;
        }
    }

    ::DebugActiveProcessStop(pid);
    ::CloseHandle(hProcess);
    return result;
}

static inline bool OpenWxWithMultiBoxing_Windows(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_crack::POpenWxWithMultiBoxingResult pResult)
{
    // check execute file path exist and is valid
    if (!wb_file::IsPathExists(wxEnvInfo.executeAbsPath)) {
        return false;
    }

    // create process and attach it
    wb_process::PID pid = wb_process::StartProcess(wxEnvInfo.executeAbsPath, true);
    if (!pid) {
        return false;
    }

    // debug loop
    if (!OpenWxWithMultiBoxing_DebugLoop(wxEnvInfo, wxApiFeatures, pid, pResult)) {
        return false;
    }

    return true;
}

#elif WXBOX_IN_MAC_OS

static inline bool OpenWxWithMultiBoxing_Mac(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_crack::POpenWxWithMultiBoxingResult pResult)
{
    throw std::exception("OpenWxWithMultiBoxing_Mac stub");
    return false;
}

#endif

bool wxbox::crack::OpenWxWithMultiBoxing(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, POpenWxWithMultiBoxingResult pResult)
{
#if WXBOX_IN_WINDOWS_OS
    return OpenWxWithMultiBoxing_Windows(wxEnvInfo, wxApiFeatures, pResult);
#elif WXBOX_IN_MAC_OS
    return OpenWxWithMultiBoxing_Mac(wxEnvInfo, wxApiFeatures, pResult);
#endif
}