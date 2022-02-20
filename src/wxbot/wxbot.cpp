#include <wxbot.hpp>

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
    if (!running) {
        return;
    }

    // stop plugin virtual machine
    wb_plugin::StopPluginVirtualMachine();

    // stop wxbox client
    client->Stop();

    running = false;
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

    // suspend all other threads
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());

    // execute hook
    ExecuteHookWeChat(true);

    // register intercept handlers
    RegisterInterceptHanlders();

    // resume all other threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // deref internal allocator
    wb_memory::deinit_internal_allocator();
    return true;
}

void wxbot::WxBot::UnHookWeChat()
{
    // init internal allocator's heap
    wb_memory::init_internal_allocator();

    // suspend all other threads
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());

    // execute unhook
    ExecuteHookWeChat(false);

    // unregister intercept handlers
    UnRegisterInterceptHanlders();

    // resume all other threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // deref internal allocator
    wb_memory::deinit_internal_allocator();

    // release hook point mem resources
    ReleasePreHookWeChatHookPoint();
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
}

void wxbot::WxBot::UnRegisterInterceptHanlders()
{
    wb_crack::UnRegisterWeChatExitHandler();
}

// must avoid system calling and memory alloc&free calling
void wxbot::WxBot::ExecuteHookWeChat(bool hook)
{
    wb_process::CallFrameHitTestItemVector testPoints;
    for (auto hookPoint : hookPoints) {
        if (hookPoint) {
            testPoints.push_back(wb_process::CallFrameHitTestItem{hookPoint, wb_crack::HOOK_OPCODE_LENGTH});
        }
    }

    for (;;) {
        auto hittedPoints = wb_process::HitTestAllOtherThreadCallFrame(testPoints);
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

        if (testPoints.empty()) {
            break;
        }

        wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());
    }
}

//
// WeChat Intercept Handler
//

void wxbot::WxBot::WeChatExitHandler()
{
    Stop();
}

//
// WxBoxClient & PluginVirtualMachine EventHandler
//

void wxbot::WxBot::WxBoxClientEventHandler(wxbot::WxBotMessage message)
{
    if (message.type == wxbot::WxBotMessageType::WxBoxClientStatusChange) {
        //spdlog::info("WxBoxClient status change, oldStatus<{}>, newStatus<{}>", ParseStatus(message.u.wxBoxClientStatus.oldStatus), ParseStatus(message.u.wxBoxClientStatus.newStatus));
    }
    else if (message.type == wxbot::WxBotMessageType::WxBoxRequestOrResponse) {
        switch (message.u.wxBoxControlPacket.type()) {
            case wxbox::ControlPacketType::PROFILE_REQUEST: {
                ResponseProfile();
                break;
            }
            case wxbox::ControlPacketType::UNINJECT_WXBOT_REQUEST: {
                Stop();
                break;
            }
        }
    }
}

void wxbot::WxBot::PluginVirtualMachineEventHandler(wb_plugin::PluginVirtualMachineEventPtr event)
{
}

//
// WxBoxClient Wrapper Response Methods
//

void wxbot::WxBot::ResponseProfile()
{
    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::PROFILE_RESPONSE);
    msg.u.wxBotControlPacket.mutable_profileresponse()->set_wxid("<is a wxid for test>");
    if (client) {
        client->PushMessageAsync(std::move(msg));
    }
}

//
// wxbot routine
//

static void WxBotRoutine(std::unique_ptr<wb_crack::WxBotEntryParameter> args)
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

    // unload wxbot module
    wb_crack::UnInjectWxBotBySelf();
}

WXBOT_PUBLIC_API void WxBotEntry(wb_crack::PWxBotEntryParameter args)
{
    if (!args) {
        wb_crack::UnInjectWxBotBySelf();
        return;
    }

    // duplicate wxbot entry parameter
    std::unique_ptr<wb_crack::WxBotEntryParameter> duplicatedArgs = std::make_unique<wb_crack::WxBotEntryParameter>();
    if (!duplicatedArgs) {
        wb_crack::UnInjectWxBotBySelf();
        return;
    }
    std::memcpy(duplicatedArgs.get(), args, sizeof(wb_crack::WxBotEntryParameter));

    // start wxbot
    std::thread(WxBotRoutine, std::move(duplicatedArgs)).detach();
}