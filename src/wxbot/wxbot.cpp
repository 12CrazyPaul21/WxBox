#include <wxbot.hpp>

#define UNINJECT_WXBOT_BY_SELF_CHECK_OVERTIME 5000
#define UNINJECT_WXBOT_BY_SELF_WATCH_INTERVAL 1000
#define WXBOT_HOOK_CHECK_OVERTIME 5000
#define WXBOT_HOOK_WATCH_INTERVAL 1000

static std::atomic_int g_singleton = 0;

//
// WxBot
//

bool wxbot::WxBot::Initialize()
{
#define WXBOT_INIT_FAILED() \
    {                       \
        inited = false;     \
        return false;       \
    }

    bool alreadyInited = false;
    inited.compare_exchange_strong(alreadyInited, true);

    if (alreadyInited) {
        return false;
    }

    if (!args) {
        WXBOT_INIT_FAILED();
    }

    // build WxBoxClient
    client = new wxbot::WxBoxClient(args->wxbox_server_uri);
    client->SetRetryInterval(args->wxbot_reconnect_interval);
    if (!client) {
        WXBOT_INIT_FAILED();
    }

    return true;
}

bool wxbot::WxBot::Startup()
{
    if (!inited) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);
    if (running) {
        return false;
    }

    // start plugin virtual machine
    wb_plugin::PluginVirtualMachineStartupInfo startupInfo;
    startupInfo.pluginPath      = args->plugins_root;
    startupInfo.longTaskTimeout = args->plugin_long_task_timeout;
    startupInfo.callback        = std::bind(&WxBot::PluginVirtualMachineEventHandler, this, std::placeholders::_1);
    if (!wb_plugin::StartPluginVirtualMachine(&startupInfo)) {
        return false;
    }

    // start wxbox client
    client->RegisterWxBotCallback(std::bind(&WxBot::WxBoxClientEventHandler, this, std::placeholders::_1));
    if (!client->Start()) {
        wb_plugin::StopPluginVirtualMachine();
        return false;
    }

    // init crack environment
    wb_crack::InitWeChatApiCrackEnvironment(args);

    running = true;
    return true;
}

void wxbot::WxBot::Wait()
{
    if (!running) {
        return;
    }

    client->Wait();
}

void wxbot::WxBot::Stop()
{
    bool isRunning = true;
    running.compare_exchange_strong(isRunning, false);

    if (!isRunning) {
        return;
    }

    wb_process::async_task([this]() {
        // stop plugin virtual machine
        wb_plugin::StopPluginVirtualMachine();

        // stop wxbox client
        client->Stop();
    });
}

void wxbot::WxBot::Shutdown()
{
    if (!inited || running) {
        return;
    }

    // destroy wxbox client
    if (client) {
        delete client;
        client = nullptr;
    }

    // clear context
    google::protobuf::ShutdownProtobufLibrary();

    inited = false;
}

bool wxbot::WxBot::HookWeChat()
{
    if (!args) {
        return false;
    }

    // execute pre intercept hook
    PreHookWeChat();

    // init internal allocator's heap
    wb_memory::init_internal_allocator();

    // start suspend watch dog
    auto watchDogTid = wb_process::StartSuspendLockWatchDog(WXBOT_HOOK_WATCH_INTERVAL);

    // suspend all other threads
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId(), watchDogTid);

    // execute hook
    ExecuteHookWeChat(true, WXBOT_HOOK_CHECK_OVERTIME, watchDogTid, true);

    // register intercept handlers
    RegisterInterceptHanlders();

    // resume all other threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // stop suspend watch dog
    wb_process::StopSuspendLockWatchDog();

    // deref internal allocator
    wb_memory::deinit_internal_allocator();
    return true;
}

void wxbot::WxBot::UnHookWeChat()
{
    // init internal allocator's heap
    wb_memory::init_internal_allocator();

    // start suspend watch dog
    auto watchDogTid = wb_process::StartSuspendLockWatchDog(WXBOT_HOOK_WATCH_INTERVAL);

    // suspend all other threads
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId(), watchDogTid);

    // execute unhook
    ExecuteHookWeChat(false, WXBOT_HOOK_CHECK_OVERTIME, watchDogTid, true);

    // unregister intercept handlers
    UnRegisterInterceptHanlders();

    // resume all other threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // stop suspend watch dog
    wb_process::StopSuspendLockWatchDog();

    // deref internal allocator
    wb_memory::deinit_internal_allocator();

    // release hook point mem resources
    ReleasePreHookWeChatHookPoint();
}

void wxbot::WxBot::log(wxbox::WxBotLogLevel level, const char* format, ...)
{
    if (!format) {
        return;
    }

    char    message[1024] = {0};
    va_list vargs;
    va_start(vargs, format);
    vsnprintf(message, sizeof(message), format, vargs);
    va_end(vargs);

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::LOG_REQUEST);

    auto logRequest = msg.u.wxBotControlPacket.mutable_logrequest();
    logRequest->set_level(level);
    logRequest->set_msg(message);

    client->PushMessageAsync(std::move(msg));
}

//
// internal
//

void wxbot::WxBot::PreHookWeChat()
{
    const wb_crack::WxApis& wxApis = args->wechat_apis;

    //
    // execute pre intercept
    //

    wb_crack::PreInterceptWeChatExit(wxApis);
    wb_crack::PreInterceptWeChatLogout(wxApis);
    wb_crack::PreInterceptWeChatLogin(wxApis);
    wb_crack::PreInterceptWeChatHandleRawMessage(wxApis);
    wb_crack::PreInterceptWeChatHandleReceviedMessages(wxApis);
    wb_crack::PreInterceptWeChatSendTextMessage(wxApis);

    //
    // record hook points
    //

    hookPoints.clear();

    if (wxApis.CloseLoginWnd) {
        hookPoints.push_back((void*)wxApis.CloseLoginWnd);
    }
    if (wxApis.LogoutAndExitWeChat) {
        hookPoints.push_back((void*)wxApis.LogoutAndExitWeChat);
    }
    if (wxApis.Logouted) {
        hookPoints.push_back((void*)wxApis.Logouted);
    }
    if (wxApis.LogoutedByMobile) {
        hookPoints.push_back((void*)wxApis.LogoutedByMobile);
    }
    if (wxApis.Logined) {
        hookPoints.push_back((void*)wxApis.Logined);
    }
    if (wxApis.HandleRawMessages) {
        hookPoints.push_back((void*)wxApis.HandleRawMessages);
    }
    if (wxApis.HandleReceivedMessages) {
        hookPoints.push_back((void*)wxApis.HandleReceivedMessages);
    }
    if (wxApis.WXSendTextMessage) {
        hookPoints.push_back((void*)wxApis.WXSendTextMessage);
    }
}

void wxbot::WxBot::ReleasePreHookWeChatHookPoint()
{
    for (auto hookPoint : hookPoints) {
        wb_hook::ReleasePreInProcessInterceptItem(hookPoint);
    }

    hookPoints.clear();
}

void wxbot::WxBot::RegisterInterceptHanlders()
{
    wb_crack::RegisterWeChatExitHandler(std::bind(&wxbot::WxBot::WeChatExitHandler, this));
    wb_crack::RegisterWeChatLogoutHandler(std::bind(&wxbot::WxBot::WeChatLogoutHandler, this));
    wb_crack::RegisterWeChatLoginHandler(std::bind(&wxbot::WxBot::WeChatLoginHandler, this));
    wb_crack::RegisterWeChatRawMessageHandler(std::bind(&wxbot::WxBot::WeChatRawMessageHandler, this, std::placeholders::_1, std::placeholders::_2));
    wb_crack::RegisterWeChatReceviedMessagesHandler(std::bind(&wxbot::WxBot::WeChatReceivedMessagesHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    wb_crack::RegisterWeChatSendTextMessageHandler(std::bind(&wxbot::WxBot::WeChatSendTextMessageHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void wxbot::WxBot::UnRegisterInterceptHanlders()
{
    wb_crack::UnRegisterWeChatExitHandler();
    wb_crack::UnRegisterWeChatLogoutHandler();
    wb_crack::UnRegisterWeChatLoginHandler();
    wb_crack::UnRegisterWeChatRawMessageHandler();
    wb_crack::UnRegisterWeChatReceviedMessagesHandler();
    wb_crack::UnRegisterWeChatSendTextMessageHandler();
}

// must avoid system calling and memory alloc&free calling
void wxbot::WxBot::ExecuteHookWeChat(bool hook, std::time_t msOvertime, wb_process::TID watchDogTid, bool forced)
{
    wb_process::CallFrameHitTestItemVector testPoints;
    for (auto hookPoint : hookPoints) {
        if (hookPoint) {
            testPoints.push_back(wb_process::CallFrameHitTestItem{hookPoint, wb_crack::HOOK_OPCODE_LENGTH});
        }
    }

    auto pid       = wb_process::GetCurrentProcessId();
    auto tid       = wb_process::GetCurrentThreadId();
    auto timestamp = wb_process::GetCurrentTimestamp(true);

    for (;;) {
        auto hittedPoints = wb_process::HitTestAllOtherThreadCallFrame(testPoints, watchDogTid);
        for (auto it = testPoints.begin(); it != testPoints.end();) {
            void* hookPoint = it->addr;
            if (std::find(hittedPoints.begin(), hittedPoints.end(), (ucpulong_t)hookPoint) == hittedPoints.end()) {
                hook ? wb_hook::ExecuteInProcessIntercept(hookPoint) : wb_hook::RevokeInProcessHook(hookPoint);
                it = testPoints.erase(it);
                continue;
            }
            else {
                ++it;
            }
        }

        if (testPoints.empty() || wb_process::GetCurrentTimestamp(true) - timestamp > msOvertime) {
            break;
        }

        wb_process::ResumeAllThread(pid);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wb_process::SuspendAllOtherThread(pid, tid, watchDogTid);

        // touch watch dog
        wb_process::TouchSuspendLockWatchDog();
    }

    if (!forced) {
        return;
    }

    for (auto it = testPoints.begin(); it != testPoints.end();) {
        void* hookPoint = it->addr;
        hook ? wb_hook::ExecuteInProcessIntercept(hookPoint) : wb_hook::RevokeInProcessHook(hookPoint);
    }
}

//
// WxBoxClient & PluginVirtualMachine EventHandler
//

void wxbot::WxBot::WxBoxClientEventHandler(wxbot::WxBotMessage message)
{
    switch (message.type) {
        case wxbot::WxBotMessageType::WxBoxClientStatusChange:
            break;

        case wxbot::WxBotMessageType::WxBoxRequestOrResponse:
            WxBoxRequestOrResponseHandler(message);
            break;
    }
}

void wxbot::WxBot::WxBoxRequestOrResponseHandler(wxbot::WxBotMessage& message)
{
    switch (message.u.wxBoxControlPacket.type()) {
        case wxbox::ControlPacketType::INJECT_ARGS_REQUEST: {
            ResponseInjectArgs();
            break;
        }

        case wxbox::ControlPacketType::PROFILE_REQUEST: {
            ResponseProfile();
            break;
        }

        case wxbox::ControlPacketType::UNINJECT_WXBOT_REQUEST: {
            Stop();
            break;
        }

        case wxbox::ControlPacketType::LOGOUT_WECHAT_REQUEST: {
            if (wb_crack::IsLoign()) {
                wb_crack::Logout();
            }
            break;
        }

        case wxbox::ControlPacketType::ALL_CONTACT_REQUEST: {
            ResponseAllContact();
            break;
        }

        case wxbox::ControlPacketType::EXECUTE_PLUGIN_SCRIPT_REQUEST: {
            ExecutePluginScript(message.u.wxBoxControlPacket.mutable_executepluginscriptrequest()->statement());
            break;
        }

        case wxbox::ControlPacketType::CHANGE_CONFIG_REQUEST: {
            auto changeConfigRequest = message.u.wxBoxControlPacket.mutable_changeconfigrequest();
            if (!changeConfigRequest) {
                break;
            }

            args->avoidRevokeMessage        = changeConfigRequest->avoidrevokemessage();
            args->enableRawMessageHook      = changeConfigRequest->enablerawmessagehook();
            args->enableSendTextMessageHook = changeConfigRequest->enablesendtextmessagehook();
            args->wxbot_reconnect_interval  = changeConfigRequest->wxboxclientreconnectinterval();
            args->plugin_long_task_timeout  = changeConfigRequest->pluginlongtasktimeout();

            client->SetRetryInterval(args->wxbot_reconnect_interval);
            wb_plugin::ChangeLongTaskTimeout(args->plugin_long_task_timeout);

            if (changeConfigRequest->serveruri().compare(args->wxbox_server_uri)) {
                strcpy_s(args->wxbox_server_uri, sizeof(args->wxbox_server_uri), changeConfigRequest->serveruri().data());
                client->ChangeServerURI(args->wxbox_server_uri);
            }
            break;
        }
    }
}

void wxbot::WxBot::PluginVirtualMachineEventHandler(wb_plugin::PluginVirtualMachineEventPtr event)
{
    switch (event->type) {
        case wb_plugin::PluginVirtualMachineEventType::ExecuteResult: {
            PluginExecuteResultEventHandler(wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::ExecuteResult>(event));
            break;
        }

        case wb_plugin::PluginVirtualMachineEventType::PluginToHost: {
            PluginToHostEventHandler(wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::PluginToHost>(event));
            break;
        }
    }
}

//
// wxbot routine
//

static void WxBotRoutine(wb_crack::WxBotEntryParameterPtr args)
{
    wxbot::WxBot bot(std::move(args));

    // init
    if (!bot.Initialize()) {
        goto _Finish;
    }

    // execute hook
    if (!bot.HookWeChat()) {
        bot.Shutdown();
        goto _Finish;
    }

    // start
    if (!bot.Startup()) {
        bot.Shutdown();
        bot.UnHookWeChat();
        goto _Finish;
    }

    // wait for wxbot finish
    bot.Wait();

    // deinit
    bot.Shutdown();

    // execute unhook
    bot.UnHookWeChat();

_Finish:

    // deinit crack environment
    wb_crack::DeInitWeChatApiCrackEnvironment();

    // unload wxbot module
    wb_crack::UnInjectWxBotBySelf(UNINJECT_WXBOT_BY_SELF_CHECK_OVERTIME, UNINJECT_WXBOT_BY_SELF_WATCH_INTERVAL, false);
    --g_singleton;
}

WXBOT_PUBLIC_API void WxBotEntry(wb_crack::PWxBotEntryParameter args)
{
    int alreadyInited = 0;
    g_singleton.compare_exchange_strong(alreadyInited, 1);
    if (alreadyInited) {
        return;
    }

    if (!args) {
        wb_crack::UnInjectWxBotBySelf(UNINJECT_WXBOT_BY_SELF_CHECK_OVERTIME, UNINJECT_WXBOT_BY_SELF_WATCH_INTERVAL, false);
        --g_singleton;
        return;
    }

    // duplicate wxbot entry parameter
    wb_crack::WxBotEntryParameterPtr duplicatedArgs = std::make_shared<wb_crack::WxBotEntryParameter>();
    if (!duplicatedArgs) {
        wb_crack::UnInjectWxBotBySelf(UNINJECT_WXBOT_BY_SELF_CHECK_OVERTIME, UNINJECT_WXBOT_BY_SELF_WATCH_INTERVAL, false);
        --g_singleton;
        return;
    }
    std::memcpy(duplicatedArgs.get(), args, sizeof(wb_crack::WxBotEntryParameter));

    // start wxbot
    std::thread(WxBotRoutine, std::move(duplicatedArgs)).detach();
}