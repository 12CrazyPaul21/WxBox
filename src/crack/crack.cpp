#include <utils/common.h>
#include <frida-gum.h>

//
// Globals
//

// hook point handler
static wb_crack::FnWeChatExitHandler g_wechat_exit_handler = nullptr;

//
// wechat intercept stubs
//

static void internal_wechat_exit_handler()
{
    if (g_wechat_exit_handler) {
        g_wechat_exit_handler();
    }
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_exit_handler_stub)
{
    __asm {
		call internal_wechat_exit_handler
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_exit_handler_stub)

//
// wxbox::crack
//

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

    // DEPRECATED :
    //
    // asm 'ret' code. [0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3](mov eax, 0; ret) can be used directly, but frida-gum's GumX86Writer is still used for assembly here
    //

    //     GumX86Writer x86Writer;
    //     guint8       x86MachineInstruction[32] = {0};
    //     guint        uInstructionSize          = 0;
    //
    // #if WXBOX_CPU_IS_X86
    //     ::gum_x86_writer_init(&x86Writer, x86MachineInstruction);
    //     ::gum_x86_writer_set_target_cpu(&x86Writer, GUM_CPU_IA32);
    //     ::gum_x86_writer_set_target_abi(&x86Writer, GUM_ABI_WINDOWS);
    //     ::gum_x86_writer_put_mov_reg_u32(&x86Writer, GUM_REG_EAX, 0);
    //     ::gum_x86_writer_put_ret(&x86Writer);
    //     uInstructionSize = ::gum_x86_writer_offset(&x86Writer);
    // #endif

    // obtain fill stream
    std::vector<uint8_t> fillStream;
    if (!wxApiFeatures.ObtainFillStream(wxEnvInfo.version, "CheckAppSingleton", fillStream)) {
        return false;
    }

    //
    // crack wechat
    //

    return wb_memory::WriteMemory(hProcess, (void*)checkAppSingletonVA, fillStream.data(), fillStream.size(), nullptr);
}

static inline bool OpenWxWithMultiBoxing_DebugLoop(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_process::PID pid, wb_crack::POpenWxWithMultiBoxingResult pResult, bool keepAttach)
{
    using FnWaitForDebugEvent = BOOL(APIENTRY*)(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);

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

    HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
    if (!hKernel32) {
        return false;
    }

    FnWaitForDebugEvent pfnWaitForDebugEvent = reinterpret_cast<FnWaitForDebugEvent>(GetProcAddress(hKernel32, "WaitForDebugEventEx"));
    if (!pfnWaitForDebugEvent) {
        pfnWaitForDebugEvent = reinterpret_cast<FnWaitForDebugEvent>(GetProcAddress(hKernel32, "WaitForDebugEvent"));
        if (!pfnWaitForDebugEvent) {
            return false;
        }
    }

    for (;;) {
        if (!pfnWaitForDebugEvent(&debugEvent, DEBUG_MODE_WAITING_INTERVAL_MS)) {
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

    if (!keepAttach) {
        ::DebugActiveProcessStop(pid);
    }

    ::CloseHandle(hProcess);
    return result;
}

static inline bool OpenWxWithMultiBoxing_Windows(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_crack::POpenWxWithMultiBoxingResult pResult, bool keepAttach)
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
    if (!OpenWxWithMultiBoxing_DebugLoop(wxEnvInfo, wxApiFeatures, pid, pResult, keepAttach)) {
        return false;
    }

    return true;
}

#elif WXBOX_IN_MAC_OS

static inline bool OpenWxWithMultiBoxing_Mac(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_crack::POpenWxWithMultiBoxingResult pResult, bool keepAttach)
{
    throw std::exception("OpenWxWithMultiBoxing_Mac stub");
    return false;
}

#endif

bool wxbox::crack::AttachWxProcess(wxbox::util::process::PID pid)
{
#if WXBOX_IN_WINDOWS_OS

    if (pid) {
        return ::DebugActiveProcess(pid);
    }

    return false;

#elif WXBOX_IN_MAC_OS
    throw std::exception("AttachWxProcess stub");
    return false;
#endif
}

void wxbox::crack::DeAttachWxProcess(wxbox::util::process::PID pid)
{
#if WXBOX_IN_WINDOWS_OS

    if (pid) {
        ::DebugActiveProcessStop(pid);
    }

#elif WXBOX_IN_MAC_OS
    throw std::exception("DeAttachWxProcess stub");
#endif
}

bool wxbox::crack::GenerateWxApis(const wb_feature::WxAPIHookPointVACollection& collection, WxApis& apis)
{
    bool success = true;
    std::memset(&apis, 0, sizeof(WxApis));

#define SET_WX_API(WX_API_NAME)                      \
    apis.WX_API_NAME = collection.get(#WX_API_NAME); \
    if (!apis.WX_API_NAME) {                         \
        success = false;                             \
    }

    SET_WX_API(CheckAppSingleton);
    SET_WX_API(FetchGlobalContactContextAddress);
    SET_WX_API(InitWeChatContactItem);
    SET_WX_API(DeinitWeChatContactItem);
    SET_WX_API(FindAndDeepCopyWeChatContactItemWithWXIDWrapper);
    SET_WX_API(FetchGlobalProfileContext);
    SET_WX_API(HandleRawMessages);
    SET_WX_API(HandleReceivedMessages);
    SET_WX_API(WXSendTextMessage);
    SET_WX_API(FetchGlobalSendMessageContext);
    SET_WX_API(WXSendFileMessage);
    SET_WX_API(CloseLoginWnd);
    SET_WX_API(LogoutAndExitWeChat);

    return success;
}

bool wxbox::crack::VerifyWxApis(const WxApis& apis)
{
#define CHECK_WX_API(WX_API_NAME) \
    if (!apis.WX_API_NAME) {      \
        return false;             \
    }

    CHECK_WX_API(CheckAppSingleton);
    CHECK_WX_API(FetchGlobalContactContextAddress);
    CHECK_WX_API(InitWeChatContactItem);
    CHECK_WX_API(DeinitWeChatContactItem);
    CHECK_WX_API(FindAndDeepCopyWeChatContactItemWithWXIDWrapper);
    CHECK_WX_API(FetchGlobalProfileContext);
    CHECK_WX_API(HandleRawMessages);
    CHECK_WX_API(HandleReceivedMessages);
    CHECK_WX_API(WXSendTextMessage);
    CHECK_WX_API(FetchGlobalSendMessageContext);
    CHECK_WX_API(WXSendFileMessage);
    CHECK_WX_API(CloseLoginWnd);
    CHECK_WX_API(LogoutAndExitWeChat);

    return true;
}

bool wxbox::crack::OpenWxWithMultiBoxing(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, POpenWxWithMultiBoxingResult pResult, bool keepAttach)
{
#if WXBOX_IN_WINDOWS_OS
    return OpenWxWithMultiBoxing_Windows(wxEnvInfo, wxApiFeatures, pResult, keepAttach);
#elif WXBOX_IN_MAC_OS
    return OpenWxWithMultiBoxing_Mac(wxEnvInfo, wxApiFeatures, pResult, keepAttach);
#endif
}

bool wxbox::crack::IsWxBotInjected(wxbox::util::process::PID pid)
{
    if (!pid) {
        return false;
    }

    wb_process::ModuleInfo moduleInfo;
    return wb_process::GetModuleInfo(pid, WXBOT_MODULE_NAME, moduleInfo);
}

bool wxbox::crack::InjectWxBot(wxbox::util::process::PID pid, const WxBotEntryParameter& parameter)
{
    if (!pid) {
        return false;
    }

    wb_inject::AddModuleSearchPath(pid, parameter.wxbot_root);
    wb_inject::MethodCallingParameter injectParameter = wb_inject::MethodCallingParameter::BuildBufferValue(const_cast<WxBotEntryParameter*>(&parameter), sizeof(parameter));
    return wb_inject::InjectModuleToProcess(pid, wb_crack::WXBOT_MODULE_NAME, wb_crack::WXBOT_ENTRY_METHOD_NAME, &injectParameter);
}

bool wxbox::crack::UnInjectWxBot(wxbox::util::process::PID pid)
{
    if (!pid) {
        return false;
    }

    return wb_inject::UnInjectModuleFromProcess(pid, wb_crack::WXBOT_MODULE_NAME);
}

bool wxbox::crack::UnInjectWxBotBySelf()
{
    return wb_inject::UnloadModuleBySelf(wb_crack::WXBOT_MODULE_NAME, wb_process::GetCurrentThreadId());
}

bool wxbox::crack::PreInterceptWeChatExit(const WxApis& wxApis)
{
    return wb_hook::PreInProcessIntercept((void*)wxApis.CloseLoginWnd, internal_wechat_exit_handler_stub) &&
           wb_hook::PreInProcessIntercept((void*)wxApis.LogoutAndExitWeChat, internal_wechat_exit_handler_stub);
}

void wxbox::crack::RegisterWeChatExitHandler(FnWeChatExitHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_exit_handler = handler;
}

void wxbox::crack::UnRegisterWeChatExitHandler()
{
    g_wechat_exit_handler = nullptr;
}