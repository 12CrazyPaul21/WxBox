#include <utils/common.h>
#include <frida-gum.h>

//
// Globals
//

static cpulong_t                        g_wechat_raw_message_type_point_offset   = 0;
static cpulong_t                        g_wechat_message_structure_known_presize = 0;
static std::mutex                       g_mutex;
static wb_crack::WxBotEntryParameterPtr g_crack_env = nullptr;

// hook point handler
static wb_crack::FnWeChatExitHandler             g_wechat_exit_handler              = nullptr;
static wb_crack::FnWeChatLogoutHandler           g_wechat_logout_handler            = nullptr;
static wb_crack::FnWeChatLoginHandler            g_wechat_login_handler             = nullptr;
static wb_crack::FnWeChatRawMessageHandler       g_wechat_raw_message_handler       = nullptr;
static wb_crack::FnWeChatReceivedMessagesHandler g_wechat_received_messages_handler = nullptr;
static wb_crack::FnWeChatSendMessageHandler      g_wechat_send_text_message_handler = nullptr;

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

static void internal_wechat_logout_handler()
{
    if (g_wechat_logout_handler) {
        g_wechat_logout_handler();
    }
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_logout_handler_stub)
{
    __asm {
		call internal_wechat_logout_handler
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_logout_handler_stub)

static void internal_wechat_login_handler()
{
    if (g_wechat_login_handler) {
        g_wechat_login_handler();
    }
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_login_handler_stub)
{
    __asm {
		call internal_wechat_login_handler
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_login_handler_stub)

static uint32_t internal_wechat_raw_message_handler(wxbox::crack::wx::WeChatMessageType type, wxbox::crack::wx::PWeChatMessage message)
{
    if (!g_wechat_raw_message_handler || !message) {
        return (uint32_t)(type);
    }

    g_wechat_raw_message_handler(type, message);
    if ((uint32_t)type != message->message_type) {
        return message->message_type;
    }

    return (uint32_t)(type);
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_raw_message_handler_stub)
{
    __asm {
        ;  // origianl ebp
		mov eax, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_EBP_OFFSET]

        ;  // original eax
		mov ebx, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_EAX_OFFSET]

        ;  // calc message start position and push
		add eax, g_wechat_raw_message_type_point_offset
		sub eax, WECHAT_MESSAGE_ITEM_TYPE_OFFSET
		push eax

        ;  // push message type
		push ebx

		;  // call raw message handler
		call internal_wechat_raw_message_handler

		;  // cover new message type
		mov [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_EAX_OFFSET+0x8], eax

		add esp, 0x8
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_raw_message_handler_stub)

static void internal_wechat_received_messages_handler(wxbox::crack::wx::PWeChatMessageCollection messageCollection)
{
    if (!g_wechat_received_messages_handler) {
        return;
    }

    if (!messageCollection || !messageCollection->begin || !messageCollection->end || messageCollection->begin == messageCollection->end) {
        return;
    }

    auto collectionSize = (ucpulong_t)messageCollection->end - (ucpulong_t)messageCollection->begin;

    //
    // use known wechat message structure presize
    //

    if (g_wechat_message_structure_known_presize) {
        auto collectionCount = collectionSize / g_wechat_message_structure_known_presize;
        g_wechat_received_messages_handler(messageCollection, collectionCount, g_wechat_message_structure_known_presize);
        return;
    }

    //
    // unknown version wechat message structure size
    //

    // calc predefine WeChatMessageStructure Size
    auto predefineWeChatMessageSize = (g_crack_env && g_crack_env->wechat_datastructure_supplement.weChatMessageStructureSize)
                                          ? g_crack_env->wechat_datastructure_supplement.weChatMessageStructureSize
                                          : sizeof(wb_wx::WeChatMessage);

    if (predefineWeChatMessageSize > collectionSize) {
        predefineWeChatMessageSize = collectionSize;
    }

    auto guessCollectionCount   = collectionSize / predefineWeChatMessageSize;
    auto guessWeChatMessageSize = collectionSize / guessCollectionCount;

    if (collectionSize % (guessCollectionCount * guessWeChatMessageSize) == 0) {
        // guess maybe successful
        if (guessCollectionCount == 1) {
            g_wechat_message_structure_known_presize = guessWeChatMessageSize;
        }
    }
    else {
        // guess failed
        guessCollectionCount = 1;
    }

    g_wechat_received_messages_handler(messageCollection, guessCollectionCount, guessWeChatMessageSize);
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_received_messages_handler_stub)
{
    __asm {
		mov eax, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_EAX_OFFSET]
		mov eax, [eax+0x5C]
		push eax
		call internal_wechat_received_messages_handler
		add esp, 0x4
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_received_messages_handler_stub)

static void internal_wechat_send_text_message_handler(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message)
{
    if (!wxid || !message || !wxid->str || !message->str) {
        return;
    }

    if (g_wechat_send_text_message_handler) {
        g_wechat_send_text_message_handler(wxid, message);
    }
}

BEGIN_NAKED_STD_FUNCTION(internal_wechat_send_text_message_handler_stub)
{
    __asm {
        // get wxid info
		mov edx, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_EDX_OFFSET]

        // push message info
		mov eax, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_ESP_OFFSET]
		push [eax+4]

        // push wxid info
		push edx

		call internal_wechat_send_text_message_handler
		add esp, 0x8
		ret
    }
}
END_NAKED_STD_FUNCTION(internal_wechat_send_text_message_handler_stub)

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
        moduleAbsPath = wb_string::ToNativeString((wchar_t*)dllAbsPathBuffer);
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

    DWORD oldProtect = 0;
    if (!VirtualProtectEx(hProcess, (void*)checkAppSingletonVA, fillStream.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

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
        CloseHandle(hProcess);
        return false;
    }

    FnWaitForDebugEvent pfnWaitForDebugEvent = reinterpret_cast<FnWaitForDebugEvent>(GetProcAddress(hKernel32, "WaitForDebugEventEx"));
    if (!pfnWaitForDebugEvent) {
        pfnWaitForDebugEvent = reinterpret_cast<FnWaitForDebugEvent>(GetProcAddress(hKernel32, "WaitForDebugEvent"));
        if (!pfnWaitForDebugEvent) {
            CloseHandle(hProcess);
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

    if (!keepAttach || !result) {
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

    // remove all wechat process's singleton mutex kernel object
    bool removeWeChatSingletonMutexResult = wb_platform::RemoveAllMatchKernelObject(WX_WE_CHAT_EXE, WX_WE_CHAT_SINGLETON_MUTEX_NAME);

    // create process and attach it
    wb_process::PID pid = wb_process::StartProcess(wxEnvInfo.executeAbsPath, true);
    if (!pid) {
        return false;
    }

    // debug loop
    if (!OpenWxWithMultiBoxing_DebugLoop(wxEnvInfo, wxApiFeatures, pid, pResult, keepAttach) && !removeWeChatSingletonMutexResult) {
        return false;
    }

    return true;
}

static void Logout_Windows(const ucpulong_t eventId, const void* pWeChatEventProc)
{
    void* pcontext = new ucpulong_t[16];
    std::memset(pcontext, 0, sizeof(ucpulong_t) * 16);

    __asm {
		mov ecx, pcontext
		push 0
		push 0
		push 0
		push eventId
		call pWeChatEventProc
    }

    delete[] pcontext;
}

#elif WXBOX_IN_MAC_OS

static inline bool OpenWxWithMultiBoxing_Mac(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_feature::WxApiFeatures& wxApiFeatures, wb_crack::POpenWxWithMultiBoxingResult pResult, bool keepAttach)
{
    throw std::exception("OpenWxWithMultiBoxing_Mac stub");
    return false;
}

static void Logout_Mac(const ucpulong_t eventId, const void* pWeChatEventProc)
{
    ucpulong_t context  = 0;
    void*      pcontext = &context;

    __asm {
		mov ecx, pcontext
		push 0
		push 0
		push 0
		push eventId
		call pWeChatEventProc
    }
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
    SET_WX_API(Logouted);
    SET_WX_API(LogoutedByMobile);
    SET_WX_API(Logined);
    SET_WX_API(WeChatEventProc);

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
    CHECK_WX_API(Logouted);
    CHECK_WX_API(LogoutedByMobile);
    CHECK_WX_API(Logined);
    CHECK_WX_API(WeChatEventProc);

    return true;
}

bool wxbox::crack::IsFullFeaturesValid(const WxBotEntryParameter& parameter)
{
    const ucpulong_t* apiCursor        = reinterpret_cast<const ucpulong_t*>(&parameter.wechat_apis);
    const ucpulong_t* supplementCursor = reinterpret_cast<const ucpulong_t*>(&parameter.wechat_datastructure_supplement);

    for (int i = 0; i < sizeof(WxApis) / sizeof(ucpulong_t); i++) {
        if (!*apiCursor++) {
            return false;
        }
    }

    for (int i = 0; i < sizeof(wxbox::crack::feature::WxDataStructSupplement) / sizeof(ucpulong_t); i++) {
        if (!*supplementCursor++) {
            return false;
        }
    }

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
    wb_process::ModuleInfo moduleInfo;
    if (!wb_process::GetModuleInfo(wb_process::GetCurrentProcessId(), wb_crack::WXBOT_MODULE_NAME, moduleInfo)) {
        return false;
    }

    wb_memory::init_internal_allocator();
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());

    while (wb_process::HitTestAllOtherThreadCallFrame(moduleInfo.pModuleBaseAddr, moduleInfo.uModuleSize)) {
        wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());
    }

    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());
    wb_memory::deinit_internal_allocator();

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

bool wxbox::crack::PreInterceptWeChatLogout(const WxApis& wxApis)
{
    return wb_hook::PreInProcessIntercept((void*)wxApis.Logouted, internal_wechat_logout_handler_stub) &&
           wb_hook::PreInProcessIntercept((void*)wxApis.LogoutedByMobile, internal_wechat_logout_handler_stub);
}

void wxbox::crack::RegisterWeChatLogoutHandler(FnWeChatLogoutHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_logout_handler = handler;
}

void wxbox::crack::UnRegisterWeChatLogoutHandler()
{
    g_wechat_logout_handler = nullptr;
}

bool wxbox::crack::PreInterceptWeChatLogin(const WxApis& wxApis)
{
    return wb_hook::PreInProcessIntercept((void*)wxApis.Logined, internal_wechat_login_handler_stub);
}

void wxbox::crack::RegisterWeChatLoginHandler(FnWeChatLoginHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_login_handler = handler;
}

void wxbox::crack::UnRegisterWeChatLoginHandler()
{
    g_wechat_login_handler = nullptr;
}

bool wxbox::crack::PreInterceptWeChatHandleRawMessage(const WxApis& wxApis)
{
    if (!wxApis.HandleRawMessages) {
        return false;
    }

#if WXBOX_CPU_IS_X86
    g_wechat_raw_message_type_point_offset = *(reinterpret_cast<signed long*>(wxApis.HandleRawMessages) - 1);
#else
    g_wechat_raw_message_type_point_offset = *(reinterpret_cast<signed long*>(wxApis.HandleRawMessages) - 1);
#endif

    return wb_hook::PreInProcessIntercept((void*)wxApis.HandleRawMessages, internal_wechat_raw_message_handler_stub);
}

void wxbox::crack::RegisterWeChatRawMessageHandler(FnWeChatRawMessageHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_raw_message_handler = handler;
}

void wxbox::crack::UnRegisterWeChatRawMessageHandler()
{
    g_wechat_raw_message_handler = nullptr;
}

bool wxbox::crack::PreInterceptWeChatHandleReceviedMessages(const WxApis& wxApis)
{
    return wb_hook::PreInProcessIntercept((void*)wxApis.HandleReceivedMessages, internal_wechat_received_messages_handler_stub);
}

void wxbox::crack::RegisterWeChatReceviedMessagesHandler(FnWeChatReceivedMessagesHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_received_messages_handler = handler;
}

void wxbox::crack::UnRegisterWeChatReceviedMessagesHandler()
{
    g_wechat_received_messages_handler = nullptr;
}

bool wxbox::crack::PreInterceptWeChatSendTextMessage(const WxApis& wxApis)
{
    return wb_hook::PreInProcessIntercept((void*)wxApis.WXSendTextMessage, internal_wechat_send_text_message_handler_stub);
}

void wxbox::crack::RegisterWeChatSendTextMessageHandler(FnWeChatSendMessageHandler handler)
{
    if (!handler) {
        return;
    }

    g_wechat_send_text_message_handler = handler;
}

void wxbox::crack::UnRegisterWeChatSendTextMessageHandler()
{
    g_wechat_send_text_message_handler = nullptr;
}

//
// wechat api
//

void wxbox::crack::InitWeChatApiCrackEnvironment(WxBotEntryParameterPtr& args)
{
    if (!args) {
        return;
    }

    g_crack_env = args;

    //
    // record known version wechat message structure size
    //

    // known version 3.4.5.27(0x278), 3.5.0.46(0x290)
    if (!strcmp(g_crack_env->wechat_version, "3.4.5.27")) {
        g_wechat_message_structure_known_presize = 0x278;
    }
    else if (!strcmp(g_crack_env->wechat_version, "3.5.0.46")) {
        g_wechat_message_structure_known_presize = 0x290;
    }
    else {
        g_wechat_message_structure_known_presize = 0x0;
    }
}

void wxbox::crack::DeInitWeChatApiCrackEnvironment()
{
    g_crack_env = nullptr;
}

uint8_t* wxbox::crack::FetchWeChatGlobalProfileContext()
{
    if (!g_crack_env || !g_crack_env->wechat_apis.FetchGlobalProfileContext) {
        return nullptr;
    }

    return (((uint8_t * (*)()) g_crack_env->wechat_apis.FetchGlobalProfileContext)());
}

bool wxbox::crack::IsLoign()
{
    uint8_t* globalProfileContext = FetchWeChatGlobalProfileContext();
    if (!globalProfileContext || !g_crack_env || !g_crack_env->wechat_datastructure_supplement.profileItemOffset.WeChatNumber) {
        return false;
    }

    return *(globalProfileContext + g_crack_env->wechat_datastructure_supplement.profileItemOffset.WeChatNumber) != '\0';
}

bool wxbox::crack::FetchProfile(wxbox::crack::wx::WeChatProfile& profile)
{
    profile.logined  = false;
    profile.nickname = "";
    profile.wxnumber = "";
    profile.wxid     = "";

    if (!g_crack_env) {
        return false;
    }
    const wxbox::crack::feature::WxDataStructSupplement& wxDataSturctsupplement = g_crack_env->wechat_datastructure_supplement;

    uint8_t* globalProfileContext = FetchWeChatGlobalProfileContext();
    if (!globalProfileContext || !wxDataSturctsupplement.profileItemOffset.WeChatNumber || !wxDataSturctsupplement.profileItemOffset.Wxid) {
        return false;
    }

    profile.logined = *(globalProfileContext + wxDataSturctsupplement.profileItemOffset.WeChatNumber) != '\0';
    if (profile.logined) {
        profile.nickname = (char*)(globalProfileContext + wxDataSturctsupplement.profileItemOffset.NickName);
        profile.wxnumber = (char*)(globalProfileContext + wxDataSturctsupplement.profileItemOffset.WeChatNumber);

        auto wxidRef = (char**)(globalProfileContext + wxDataSturctsupplement.profileItemOffset.Wxid);
        if (wxidRef && *wxidRef) {
            profile.wxid = (char*)(*wxidRef);
        }
    }

    return true;
}

bool wxbox::crack::Logout()
{
    if (!g_crack_env) {
        return false;
    }
    const WxApis&                             wxApis                 = g_crack_env->wechat_apis;
    const wb_feature::WxDataStructSupplement& wxDataSturctsupplement = g_crack_env->wechat_datastructure_supplement;

    if (!wxApis.WeChatEventProc || !wxDataSturctsupplement.logoutTriggerEventId) {
        return false;
    }

#if WXBOX_IN_WINDOWS_OS
    std::async(Logout_Windows, wxDataSturctsupplement.logoutTriggerEventId, (void*)wxApis.WeChatEventProc).wait();
#else
    std::async(Logout_Mac, wxDataSturctsupplement.logoutTriggerEventId, (void*)wxApis.WeChatEventProc).wait();
#endif
    return true;
}

uint8_t* wxbox::crack::FetchWeChatGlobalContactContextAddress()
{
    if (!g_crack_env || !g_crack_env->wechat_apis.FetchGlobalContactContextAddress) {
        return nullptr;
    }

    return (((uint8_t * (*)()) g_crack_env->wechat_apis.FetchGlobalContactContextAddress)());
}

uint8_t* wxbox::crack::FetchContactHeaderAddress()
{
    if (!g_crack_env) {
        return false;
    }
    const wb_feature::WxDataStructSupplement& wxDataSturctsupplement = g_crack_env->wechat_datastructure_supplement;

    uint8_t* globalContactContext = FetchWeChatGlobalContactContextAddress();
    if (!globalContactContext || !wxDataSturctsupplement.weChatContactHeaderItemOffset) {
        return nullptr;
    }

    uint8_t** ppHeader = reinterpret_cast<uint8_t**>(globalContactContext + wxDataSturctsupplement.weChatContactHeaderItemOffset);
    if (!ppHeader) {
        return nullptr;
    }

    return *ppHeader;
}

bool wxbox::crack::InitWeChatContactItem(uint8_t* contactItem)
{
    if (!g_crack_env) {
        return false;
    }

    void* pInitWeChatContactItem = reinterpret_cast<void*>(g_crack_env->wechat_apis.InitWeChatContactItem);
    if (!pInitWeChatContactItem || !contactItem) {
        return false;
    }

    __asm {
		mov ecx, contactItem
		call pInitWeChatContactItem
    }

    return true;
}

bool wxbox::crack::DeinitWeChatContactItem(uint8_t* contactItem)
{
    if (!g_crack_env) {
        return false;
    }

    void* pDeinitWeChatContactItem = reinterpret_cast<void*>(g_crack_env->wechat_apis.DeinitWeChatContactItem);
    if (!pDeinitWeChatContactItem || !contactItem) {
        return false;
    }

    __asm {
		mov ecx, contactItem
		call pDeinitWeChatContactItem
    }

    return true;
}

static inline bool Inner_CollectSingleContact(uint8_t* contactItemData, wxbox::crack::wx::WeChatContact& contact)
{
    wb_wx::PWeChatWString wxidStringStruct     = WECHAT_CONTACT_ITEM_WXID(contactItemData);
    wb_wx::PWeChatWString wxnumberStringStruct = WECHAT_CONTACT_ITEM_WXNUMBER(contactItemData);
    wb_wx::PWeChatWString remarkStringStruct   = WECHAT_CONTACT_ITEM_REMARK(contactItemData);
    wb_wx::PWeChatWString nicknameStringStruct = WECHAT_CONTACT_ITEM_NICKNAME(contactItemData);

    if (wxidStringStruct && wxidStringStruct->str && wxidStringStruct->length) {
        contact.wxid = wb_string::ToUtf8String(wxidStringStruct->str);
    }

    if (wxnumberStringStruct && wxnumberStringStruct->str && wxnumberStringStruct->length) {
        contact.wxnumber = wb_string::ToUtf8String(wxnumberStringStruct->str);
    }

    if (remarkStringStruct && remarkStringStruct->str && remarkStringStruct->length) {
        contact.remark = wb_string::ToUtf8String(remarkStringStruct->str);
    }

    if (nicknameStringStruct && nicknameStringStruct->str && nicknameStringStruct->length) {
        contact.nickname = wb_string::ToUtf8String(nicknameStringStruct->str);
    }

    contact.chatroom = (!contact.wxid.empty() && contact.wxid.rfind("@chatroom") != std::string::npos);

    return true;
}

typedef struct _Inner_CollectAllContact_below_3_5_0_46_Parameter
{
    wb_wx::PWeChatContactItem_below_3_5_0_46      end;
    ucpulong_t                                    weChatContactDataBeginOffset;
    std::vector<wxbox::crack::wx::WeChatContact>& contacts;

    _Inner_CollectAllContact_below_3_5_0_46_Parameter(wb_wx::PWeChatContactItem_below_3_5_0_46 end, ucpulong_t weChatContactDataBeginOffset, std::vector<wxbox::crack::wx::WeChatContact>& contacts)
      : end(end)
      , weChatContactDataBeginOffset(weChatContactDataBeginOffset)
      , contacts(contacts)
    {
    }
} Inner_CollectAllContact_below_3_5_0_46_Parameter;

static void Inner_CollectAllContact_below_3_5_0_46_Do(wb_wx::PWeChatContactItem_below_3_5_0_46 item, Inner_CollectAllContact_below_3_5_0_46_Parameter& parameter)
{
    if (!item || item == parameter.end) {
        return;
    }

    wxbox::crack::wx::WeChatContact contact;
    if (Inner_CollectSingleContact(((uint8_t*)item) + parameter.weChatContactDataBeginOffset, contact)) {
        parameter.contacts.emplace_back(std::move(contact));
    }

    Inner_CollectAllContact_below_3_5_0_46_Do(item->left, parameter);
    Inner_CollectAllContact_below_3_5_0_46_Do(item->right, parameter);
}

static bool Inner_CollectAllContact_below_3_5_0_46(uint8_t* contactHeaderAddress, ucpulong_t weChatContactDataBeginOffset, std::vector<wxbox::crack::wx::WeChatContact>& contacts)
{
    wb_wx::PWeChatContactHeader_below_3_5_0_46 contactHeader = (wb_wx::PWeChatContactHeader_below_3_5_0_46)contactHeaderAddress;

    Inner_CollectAllContact_below_3_5_0_46_Parameter parameter((wb_wx::PWeChatContactItem_below_3_5_0_46)contactHeader, weChatContactDataBeginOffset, contacts);
    Inner_CollectAllContact_below_3_5_0_46_Do(contactHeader->begin, parameter);
    return true;
}

static bool Inner_CollectAllContact_above_3_5_0_46(uint8_t* contactHeaderAddress, ucpulong_t weChatContactDataBeginOffset, std::vector<wxbox::crack::wx::WeChatContact>& contacts)
{
    wb_wx::PWeChatContactHeader_above_3_5_0_46 contactHeader = (wb_wx::PWeChatContactHeader_above_3_5_0_46)contactHeaderAddress;
    wb_wx::PWeChatContactItem_above_3_5_0_46   cursor        = contactHeader->begin;

    while (cursor) {
        wxbox::crack::wx::WeChatContact contact;
        if (Inner_CollectSingleContact(((uint8_t*)cursor) + weChatContactDataBeginOffset, contact)) {
            contacts.emplace_back(std::move(contact));
        }

        if (cursor == contactHeader->end) {
            break;
        }

        cursor = cursor->next;
    }

    return true;
}

bool wxbox::crack::CollectAllContact(std::vector<wxbox::crack::wx::WeChatContact>& contacts)
{
    if (!g_crack_env) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    uint8_t* contactHeaderAddress = FetchContactHeaderAddress();
    if (!contactHeaderAddress || !g_crack_env->wechat_datastructure_supplement.weChatContactDataBeginOffset) {
        return false;
    }

    wb_file::VersionNumber v3_5_0_46;
    if (!wb_file::UnwindVersionNumber("3.5.0.46", v3_5_0_46)) {
        return false;
    }

    wb_file::VersionNumber versionNumber;
    if (!wb_file::UnwindVersionNumber(g_crack_env->wechat_version, versionNumber)) {
        return false;
    }

    if (versionNumber < v3_5_0_46) {
        return Inner_CollectAllContact_below_3_5_0_46(contactHeaderAddress, g_crack_env->wechat_datastructure_supplement.weChatContactDataBeginOffset, contacts);
    }
    else {
        return Inner_CollectAllContact_above_3_5_0_46(contactHeaderAddress, g_crack_env->wechat_datastructure_supplement.weChatContactDataBeginOffset, contacts);
    }
}

enum class _Inner_SearchContactRole
{
    NickName,
    WxNumber,
};

typedef struct _Inner_SearchContact_below_3_5_0_46_Parameter
{
    _Inner_SearchContactRole                 role;
    const std::string&                       pattern;
    wb_wx::PWeChatContactItem_below_3_5_0_46 end;
    ucpulong_t                               weChatContactDataBeginOffset;
    wxbox::crack::wx::WeChatContact&         contact;

    _Inner_SearchContact_below_3_5_0_46_Parameter(_Inner_SearchContactRole role, const std::string& pattern, wb_wx::PWeChatContactItem_below_3_5_0_46 end, ucpulong_t weChatContactDataBeginOffset, wxbox::crack::wx::WeChatContact& contact)
      : role(role)
      , pattern(pattern)
      , end(end)
      , weChatContactDataBeginOffset(weChatContactDataBeginOffset)
      , contact(contact)
    {
    }
} Inner_SearchContact_below_3_5_0_46_Parameter;

static bool Inner_SearchContact_below_3_5_0_46_Do(wb_wx::PWeChatContactItem_below_3_5_0_46 item, Inner_SearchContact_below_3_5_0_46_Parameter& parameter)
{
    if (!item || item == parameter.end) {
        return false;
    }

    uint8_t*              contactItemData = ((uint8_t*)item) + parameter.weChatContactDataBeginOffset;
    wb_wx::PWeChatWString pStringStruct   = parameter.role == _Inner_SearchContactRole::NickName ? WECHAT_CONTACT_ITEM_NICKNAME(contactItemData) : WECHAT_CONTACT_ITEM_WXNUMBER(contactItemData);

    if (pStringStruct && pStringStruct->str && pStringStruct->length) {
        if (!wb_string::ToUtf8String(pStringStruct->str).compare(parameter.pattern)) {
            Inner_CollectSingleContact(contactItemData, parameter.contact);
            return true;
        }
    }

    if (Inner_SearchContact_below_3_5_0_46_Do(item->left, parameter)) {
        return true;
    }

    if (Inner_SearchContact_below_3_5_0_46_Do(item->right, parameter)) {
        return true;
    }

    return false;
}

static bool Inner_SearchContact_below_3_5_0_46(_Inner_SearchContactRole role, const std::string& pattern, uint8_t* contactHeaderAddress, ucpulong_t weChatContactDataBeginOffset, wxbox::crack::wx::WeChatContact& contact)
{
    wb_wx::PWeChatContactHeader_below_3_5_0_46 contactHeader = (wb_wx::PWeChatContactHeader_below_3_5_0_46)contactHeaderAddress;

    _Inner_SearchContact_below_3_5_0_46_Parameter parameter(role, pattern, (wb_wx::PWeChatContactItem_below_3_5_0_46)contactHeader, weChatContactDataBeginOffset, contact);
    return Inner_SearchContact_below_3_5_0_46_Do(contactHeader->begin, parameter);
}

static bool Inner_SearchContact_above_3_5_0_46(_Inner_SearchContactRole role, const std::string& pattern, uint8_t* contactHeaderAddress, ucpulong_t weChatContactDataBeginOffset, wxbox::crack::wx::WeChatContact& contact)
{
    wb_wx::PWeChatContactHeader_above_3_5_0_46 contactHeader = (wb_wx::PWeChatContactHeader_above_3_5_0_46)contactHeaderAddress;
    wb_wx::PWeChatContactItem_above_3_5_0_46   cursor        = contactHeader->begin;

    while (cursor) {
        uint8_t*              contactItemData = ((uint8_t*)cursor) + weChatContactDataBeginOffset;
        wb_wx::PWeChatWString pStringStruct   = role == _Inner_SearchContactRole::NickName ? WECHAT_CONTACT_ITEM_NICKNAME(contactItemData) : WECHAT_CONTACT_ITEM_WXNUMBER(contactItemData);

        if (pStringStruct && pStringStruct->str && pStringStruct->length) {
            if (!wb_string::ToUtf8String(pStringStruct->str).compare(pattern)) {
                return Inner_CollectSingleContact(contactItemData, contact);
            }
        }

        if (cursor == contactHeader->end) {
            break;
        }

        cursor = cursor->next;
    }

    return false;
}

bool Inner_SearchContact(_Inner_SearchContactRole role, const std::string& pattern, const wb_crack::PWxBotEntryParameter args, wxbox::crack::wx::WeChatContact& contact)
{
    if (pattern.empty() || !args) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    uint8_t* contactHeaderAddress = wb_crack::FetchContactHeaderAddress();
    if (!contactHeaderAddress || !args->wechat_datastructure_supplement.weChatContactDataBeginOffset) {
        return false;
    }

    wb_file::VersionNumber v3_5_0_46;
    if (!wb_file::UnwindVersionNumber("3.5.0.46", v3_5_0_46)) {
        return false;
    }

    wb_file::VersionNumber versionNumber;
    if (!wb_file::UnwindVersionNumber(args->wechat_version, versionNumber)) {
        return false;
    }

    if (versionNumber < v3_5_0_46) {
        return Inner_SearchContact_below_3_5_0_46(role, pattern, contactHeaderAddress, args->wechat_datastructure_supplement.weChatContactDataBeginOffset, contact);
    }
    else {
        return Inner_SearchContact_above_3_5_0_46(role, pattern, contactHeaderAddress, args->wechat_datastructure_supplement.weChatContactDataBeginOffset, contact);
    }
}

bool wxbox::crack::GetContactWithNickName(const std::string& nickname, wxbox::crack::wx::WeChatContact& contact)
{
    return Inner_SearchContact(_Inner_SearchContactRole::NickName, nickname, g_crack_env.get(), contact);
}

bool wxbox::crack::GetContactWithWxNumber(const std::string& wxnumber, wxbox::crack::wx::WeChatContact& contact)
{
    return Inner_SearchContact(_Inner_SearchContactRole::WxNumber, wxnumber, g_crack_env.get(), contact);
}

static bool Inner_FindAndDeepCopyWeChatContactItemWithWXIDWrapper_below_3_5_0_46(void* pFindAndDeepCopyWeChatContactItemWithWXIDWrapper, uint8_t* globalContactContext, void* contactItem, wb_wx::PWeChatWString pWxidString)
{
    bool result = false;

    __asm {
		push contactItem
		push pWxidString
		mov ecx, globalContactContext
		call pFindAndDeepCopyWeChatContactItemWithWXIDWrapper
		mov result, al
    }

    return result;
}

static bool Inner_FindAndDeepCopyWeChatContactItemWithWXIDWrapper_above_3_5_0_46(void* pFindAndDeepCopyWeChatContactItemWithWXIDWrapper, uint8_t* globalContactContext, void* contactItem, wb_wx::PWeChatWString pWxidString)
{
    bool     result = false;
    wchar_t* p      = pWxidString->str;
    uint32_t len    = pWxidString->length;

    __asm {
		push contactItem
		push 0
		push 0
		push len
		push len
		push p
		mov ecx, globalContactContext
		call pFindAndDeepCopyWeChatContactItemWithWXIDWrapper
		mov result, al
    }

    return result;
}

bool wxbox::crack::GetContactWithWxid(const std::string& wxid, wxbox::crack::wx::WeChatContact& contact)
{
    if (wxid.empty() || !g_crack_env || !g_crack_env->wechat_apis.FindAndDeepCopyWeChatContactItemWithWXIDWrapper) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    //
    // compare wechat version number
    //

    wb_file::VersionNumber v3_5_0_46;
    if (!wb_file::UnwindVersionNumber("3.5.0.46", v3_5_0_46)) {
        return false;
    }

    wb_file::VersionNumber versionNumber;
    if (!wb_file::UnwindVersionNumber(g_crack_env->wechat_version, versionNumber)) {
        return false;
    }

    bool below_3_5_0_46 = versionNumber < v3_5_0_46;

    //
    // get global contact context
    //

    uint8_t* globalContactContext = FetchWeChatGlobalContactContextAddress();
    if (!globalContactContext || !g_crack_env->wechat_datastructure_supplement.weChatContactHeaderItemOffset) {
        return false;
    }

    //
    // allocate contact buffer
    //

    std::unique_ptr<uint8_t[]> contactBuffer = std::make_unique<uint8_t[]>(g_crack_env->wechat_datastructure_supplement.weChatContactDataBeginOffset + 0x1000);
    if (!contactBuffer) {
        return false;
    }

    // calc contact item start position
    uint8_t* contactItem = contactBuffer.get() + g_crack_env->wechat_datastructure_supplement.weChatContactDataBeginOffset;

    // init contact item
    if (!InitWeChatContactItem(contactItem)) {
        return false;
    }

    //
    // build wxid string struct
    //

    std::wstring         wWxidStr = wb_string::ToUtf8WString(wxid);
    wb_wx::WeChatWString wxidStr;
    wxidStr.str      = const_cast<wchar_t*>(wWxidStr.c_str());
    wxidStr.length   = wWxidStr.length();
    wxidStr.length2  = wxidStr.length;
    wxidStr.unknown1 = 0;
    wxidStr.unknown2 = 0;

    //
    // call FindAndDeepCopyWeChatContactItemWithWXIDWrapper
    //

    bool  success                                          = false;
    void* pFindAndDeepCopyWeChatContactItemWithWXIDWrapper = reinterpret_cast<void*>(g_crack_env->wechat_apis.FindAndDeepCopyWeChatContactItemWithWXIDWrapper);

    if (below_3_5_0_46) {
        success = Inner_FindAndDeepCopyWeChatContactItemWithWXIDWrapper_below_3_5_0_46(pFindAndDeepCopyWeChatContactItemWithWXIDWrapper, globalContactContext, contactItem, &wxidStr);
    }
    else {
        success = Inner_FindAndDeepCopyWeChatContactItemWithWXIDWrapper_above_3_5_0_46(pFindAndDeepCopyWeChatContactItemWithWXIDWrapper, globalContactContext, contactItem, &wxidStr);
    }

    if (!success) {
        DeinitWeChatContactItem(contactItem);
        return false;
    }

    // record contact
    success = Inner_CollectSingleContact(contactItem, contact);

    DeinitWeChatContactItem(contactItem);
    return success;
}

static bool Inner_SendTextMessage(const wb_crack::PWxBotEntryParameter args, const std::string& wxid, const std::string& message, const wb_wx::PChatRoomNotifyList pNotifyList, const wchar_t* messageNotifyListSuffix = nullptr)
{
    if (!args || !args->wechat_apis.WXSendTextMessage || wxid.empty() || (message.empty() && !messageNotifyListSuffix) || !pNotifyList) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    //
    // check logged-in
    //

    if (!wb_crack::IsLoign()) {
        return false;
    }

    //
    // build wxid info
    //

    std::wstring          wWxid = wb_string::ToUtf8WString(wxid);
    wb_wx::WeChatWString  wxidInfo;
    wb_wx::PWeChatWString pWxidInfo = &wxidInfo;
    wxidInfo.str                    = const_cast<wchar_t*>(wWxid.c_str());
    wxidInfo.length                 = wWxid.length();
    wxidInfo.length2                = wxidInfo.length;
    wxidInfo.unknown1               = 0;
    wxidInfo.unknown2               = 0;

    //
    // build message info
    //

    std::wstring wMessage = wb_string::ToUtf8WString(message);
    if (messageNotifyListSuffix) {
        wMessage += messageNotifyListSuffix;
    }

    wb_wx::WeChatWString  messageInfo;
    wb_wx::PWeChatWString pMessageInfo = &messageInfo;
    messageInfo.str                    = const_cast<wchar_t*>(wMessage.c_str());
    messageInfo.length                 = wMessage.length();
    messageInfo.length2                = messageInfo.length;
    messageInfo.unknown1               = 0;
    messageInfo.unknown2               = 0;

    //
    // build fake WeChatMessageStructure structure
    //

    std::unique_ptr<uint8_t[]> fakeWeChatMessageStructure = std::make_unique<uint8_t[]>(0x1000);
    if (!fakeWeChatMessageStructure) {
        return false;
    }

    uint8_t* pFakeWeChatMessageStructure = fakeWeChatMessageStructure.get();
    std::memset(pFakeWeChatMessageStructure, 0, 0x1000);

    //
    // execute
    //

    void* result             = nullptr;
    void* pWXSendTextMessage = reinterpret_cast<void*>(args->wechat_apis.WXSendTextMessage);

    __asm {
		push 1
		push pNotifyList
		push pMessageInfo
		mov edx, pWxidInfo
		mov ecx, pFakeWeChatMessageStructure
		call pWXSendTextMessage
		add esp, 0xC
		mov result, eax
    }

    return result != nullptr;
}

bool wxbox::crack::SendTextMessage(const std::string& wxid, const std::string& message)
{
    //
    // build fake ChatRoomNotifyList structure
    //

    std::unique_ptr<uint8_t[]> fakeChatRoomNotifyList = std::make_unique<uint8_t[]>(0x1000);
    if (!fakeChatRoomNotifyList) {
        return false;
    }

    uint8_t* pFakeChatRoomNotifyList = fakeChatRoomNotifyList.get();

    //
    // execute
    //

    return Inner_SendTextMessage(g_crack_env.get(), wxid, message, reinterpret_cast<wb_wx::PChatRoomNotifyList>(pFakeChatRoomNotifyList));
}

static inline std::wstring Inner_GenerateNotifyMessage(const std::string& nickname)
{
    return L"@" + wb_string::ToUtf8WString(nickname) + L"\x2005";
}

bool wxbox::crack::SendTextMessageWithNotifyList(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists, const std::string& message)
{
    //
    // build ChatRoomNotifyList
    //

    wb_wx::ChatRoomNotifyList notifyList;
    std::memset(&notifyList, 0, sizeof(notifyList));

    std::vector<std::wstring>         vtNotifyWxidListsBuffer;
    std::vector<wb_wx::WeChatWString> vtNotifyWxidLists;
    for (auto wxid : notifyWxidLists) {
        wb_wx::WeChatWString wxidInfo;
        std::wstring         wWxid = wb_string::ToUtf8WString(wxid);
        wxidInfo.str               = const_cast<wchar_t*>(wWxid.c_str());
        wxidInfo.length            = wWxid.length();
        wxidInfo.length2           = wxidInfo.length;
        wxidInfo.unknown1          = 0;
        wxidInfo.unknown2          = 0;

        vtNotifyWxidListsBuffer.emplace_back(std::move(wWxid));
        vtNotifyWxidLists.emplace_back(wxidInfo);
    }

    if (!vtNotifyWxidLists.empty()) {
        notifyList.begin = vtNotifyWxidLists.data();
        notifyList.end   = notifyList.begin + vtNotifyWxidLists.size();
    }

    //
    // generate notify list messsage suffix
    //

    std::wstringstream                 ssMsgSuffix;
    std::map<std::string, std::string> mapNickname;
    for (auto wxid : notifyWxidLists) {
        if (mapNickname.find(wxid) != mapNickname.end()) {
            ssMsgSuffix << Inner_GenerateNotifyMessage(mapNickname.at(wxid));
            continue;
        }

        std::string nickname;
        if (wxid.compare("notify@all")) {
            wb_wx::WeChatContact contact;
            if (!GetContactWithWxid(wxid, contact)) {
                continue;
            }
            nickname = contact.nickname;
        }
        else {
            nickname = "\xE6\x89\x80\xE6\x9C\x89\xE4\xBA\xBA";
        }

        mapNickname[wxid] = nickname;
        ssMsgSuffix << Inner_GenerateNotifyMessage(nickname);
    }
    std::wstring msgSuffix = ssMsgSuffix.str();

    //
    // execute
    //

    return Inner_SendTextMessage(g_crack_env.get(), roomWxid, message, &notifyList, msgSuffix.empty() ? nullptr : msgSuffix.c_str());
}

bool wxbox::crack::SendFile(const std::string& wxid, const std::string& filePath)
{
    if (!g_crack_env || !g_crack_env->wechat_apis.WXSendFileMessage || !g_crack_env->wechat_apis.FetchGlobalSendMessageContext || wxid.empty() || filePath.empty() || !wb_file::IsPathExists(wb_string::Utf8ToNativeString(filePath))) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    //
    // check logged-in
    //

    if (!wb_crack::IsLoign()) {
        return false;
    }

    //
    // build wxid info
    //

    std::wstring          wWxid = wb_string::ToUtf8WString(wxid);
    wb_wx::WeChatWString  wxidInfo;
    wb_wx::PWeChatWString pWxidInfo = &wxidInfo;
    wxidInfo.str                    = const_cast<wchar_t*>(wWxid.c_str());
    wxidInfo.length                 = wWxid.length();
    wxidInfo.length2                = wxidInfo.length;
    wxidInfo.unknown1               = 0;
    wxidInfo.unknown2               = 0;

    //
    // build file path info
    //

    std::wstring          wFilePath = wb_string::ToUtf8WString(filePath);
    wb_wx::WeChatWString  filePathInfo;
    wb_wx::PWeChatWString pFilePathInfo = &filePathInfo;
    filePathInfo.str                    = const_cast<wchar_t*>(wFilePath.c_str());
    filePathInfo.length                 = wFilePath.length();
    filePathInfo.length2                = filePathInfo.length;
    filePathInfo.unknown1               = 0;
    filePathInfo.unknown2               = 0;

    //
    // build fake WeChatMessageStructure structure
    //

    std::unique_ptr<uint8_t[]> fakeWeChatMessageStructure = std::make_unique<uint8_t[]>(0x1000);
    if (!fakeWeChatMessageStructure) {
        return false;
    }

    uint8_t* pFakeWeChatMessageStructure = fakeWeChatMessageStructure.get();
    std::memset(pFakeWeChatMessageStructure, 0, 0x1000);

    //
    // execute
    //

    void* result                         = nullptr;
    void* pFetchGlobalSendMessageContext = reinterpret_cast<void*>(g_crack_env->wechat_apis.FetchGlobalSendMessageContext);
    void* pWXSendFileMessage             = reinterpret_cast<void*>(g_crack_env->wechat_apis.WXSendFileMessage);

    __asm {
		push 0
		push 0
		push pFilePathInfo
		push pWxidInfo

		call pFetchGlobalSendMessageContext
		mov ecx, eax

		push pFakeWeChatMessageStructure
		call pWXSendFileMessage
		mov result, eax
    }

    return result != nullptr;
}

bool wxbox::crack::SubstituteWeChatWString(wxbox::crack::wx::PWeChatWString original, const std::wstring& substitute)
{
    if (!original || !original->str || substitute.empty()) {
        return false;
    }

    auto substituteLength     = substitute.length();
    auto substituteBufferSize = (substituteLength + 1) * sizeof(wchar_t);

    if (original->length >= substituteLength) {
        std::memcpy(original->str, substitute.data(), substituteBufferSize);
        original->length  = substituteLength;
        original->length2 = original->length;
        return true;
    }

#if WXBOX_IN_WINDOWS_OS
    void* rp = HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, substituteBufferSize);
    if (!rp) {
        return false;
    }

    if (!::HeapFree(::GetProcessHeap(), 0, original->str)) {
        ::HeapFree(::GetProcessHeap(), 0, rp);
        return false;
    }

    original->str = reinterpret_cast<wchar_t*>(rp);
#else
    uint8_t* rp = realloc(original->str, substituteBufferSize);
    if (!rp) {
        return false;
    }

    original->str = reinterpret_cast<wchar_t*>(rp);
#endif

    std::memcpy(original->str, substitute.data(), substituteBufferSize);
    original->length  = substituteLength;
    original->length2 = original->length;

    return true;
}