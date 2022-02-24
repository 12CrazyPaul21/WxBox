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
    wb_crack::PreInterceptWeChatLogout(wxApis);
    wb_crack::PreInterceptWeChatLogin(wxApis);

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
}

void wxbot::WxBot::UnRegisterInterceptHanlders()
{
    wb_crack::UnRegisterWeChatExitHandler();
    wb_crack::UnRegisterWeChatLogoutHandler();
    wb_crack::UnRegisterWeChatLoginHandler();
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

void wxbot::WxBot::WeChatLogoutHandler()
{
    //
    // report logout
    //

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::PROFILE_RESPONSE);

    auto profileResponse = msg.u.wxBotControlPacket.mutable_profileresponse();
    profileResponse->set_logined(false);
    profileResponse->set_nickname("");
    profileResponse->set_wxnumber("");
    profileResponse->set_wxid("");

    client->PushMessageAsync(std::move(msg));
}

void wxbot::WxBot::WeChatLoginHandler()
{
    ResponseProfile();
}

//
// WxBoxClient & PluginVirtualMachine EventHandler
//

void wxbot::WxBot::WxBoxClientEventHandler(wxbot::WxBotMessage message)
{
    switch (message.type) {
        case wxbot::WxBotMessageType::WxBoxClientStatusChange:
            //spdlog::info("WxBoxClient status change, oldStatus<{}>, newStatus<{}>", ParseStatus(message.u.wxBoxClientStatus.oldStatus), ParseStatus(message.u.wxBoxClientStatus.newStatus));
            break;

        case wxbot::WxBotMessageType::WxBoxRequestOrResponse:
            WxBoxRequestOrResponseHandler(message);
            break;
    }
}

void wxbot::WxBot::WxBoxRequestOrResponseHandler(wxbot::WxBotMessage& message)
{
    switch (message.u.wxBoxControlPacket.type()) {
        case wxbox::ControlPacketType::PROFILE_REQUEST: {
            ResponseProfile();
            break;
        }

        case wxbox::ControlPacketType::UNINJECT_WXBOT_REQUEST: {
            Stop();
            break;
        }

        case wxbox::ControlPacketType::LOGOUT_WECHAT_REQUEST: {
            if (wb_crack::IsLoign(args->wechat_apis, args->wechat_datastructure_supplement)) {
                wb_crack::Logout(args->wechat_apis, args->wechat_datastructure_supplement);
            }
            break;
        }

        case wxbox::ControlPacketType::ALL_CONTACT_REQUEST: {
            ResponseAllContact();
            break;
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
    // get wechat user profile
    wxbox::crack::wx::WeChatProfile profile;
    wb_crack::FetchProfile(args->wechat_apis, args->wechat_datastructure_supplement, profile);

    //
    // response
    //

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::PROFILE_RESPONSE);

    auto profileResponse = msg.u.wxBotControlPacket.mutable_profileresponse();
    profileResponse->set_logined(profile.logined);
    profileResponse->set_nickname(profile.nickname);
    profileResponse->set_wxnumber(profile.wxnumber);
    profileResponse->set_wxid(profile.wxid);

    client->PushMessageAsync(std::move(msg));
}

void wxbot::WxBot::ResponseAllContact()
{
    std::vector<wxbox::crack::wx::WeChatContact> contacts;
    if (!wb_crack::CollectAllContact(args.get(), contacts)) {
        return;
    }

    if (contacts.empty()) {
        return;
    }

    //
    // response
    //

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::ALL_CONTACT_RESPONSE);

    auto allContactResponse = msg.u.wxBotControlPacket.mutable_allcontactresponse();
    if (!allContactResponse) {
        return;
    }

    for (auto contact : contacts) {
        auto pContact = allContactResponse->add_contacts();
        if (!pContact) {
            continue;
        }

        pContact->set_chatroom(contact.chatroom);
        pContact->set_nickname(contact.nickname);
        pContact->set_wxnumber(contact.wxnumber);
        pContact->set_wxid(contact.wxid);
        pContact->set_remark(contact.remark);
    }

    client->PushMessageAsync(std::move(msg));
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