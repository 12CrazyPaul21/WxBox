#include <wxbot.hpp>

const char* ParseStatus(wxbot::WxBoxClientStatus status)
{
    switch (status) {
        case wxbot::WxBoxClientStatus::Uninit:
            return "Uninit";

        case wxbot::WxBoxClientStatus::Started:
            return "Started";

        case wxbot::WxBoxClientStatus::ConnectWxBoxServerFailed:
            return "ConnectWxBoxServerFailed";

        case wxbot::WxBoxClientStatus::ConnectWxBoxServerSuccess:
            return "ConnectWxBoxServerSuccess";

        case wxbot::WxBoxClientStatus::ConnectionLost:
            return "ConnectionLost";

        case wxbot::WxBoxClientStatus::Stopped:
            return "Stopped";

        case wxbot::WxBoxClientStatus::DoReConnect:
            return "DoReConnect";

        default:
            return "";
    }
}

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
#define WXBOT_STARTUP_FAILED() \
    {                          \
        running = false;       \
        return false;          \
    }

    if (!inited) {
        return false;
    }

    bool alreadyRunning = false;
    running.compare_exchange_strong(alreadyRunning, true);

    if (alreadyRunning) {
        return false;
    }

    // start plugin virtual machine
    wb_plugin::PluginVirtualMachineStartupInfo startupInfo;
    startupInfo.pluginPath      = args->plugins_root;
    startupInfo.longTaskTimeout = args->plugin_long_task_timeout;
    startupInfo.callback        = std::bind(&WxBot::PluginVirtualMachineEventHandler, this, std::placeholders::_1);
    if (!wb_plugin::StartPluginVirtualMachine(&startupInfo)) {
        WXBOT_STARTUP_FAILED();
    }

    // start wxbox client
    client->RegisterWxBotCallback(std::bind(&WxBot::WxBoxClientEventHandler, this, std::placeholders::_1));
    if (!client->Start()) {
        wb_plugin::StopPluginVirtualMachine();
        WXBOT_STARTUP_FAILED();
    }

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
    return true;
}

void wxbot::WxBot::UnHookWeChat()
{
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
        if (message.u.wxBoxControlPacket.type() == wxbox::ControlPacketType::PROFILE_REQUEST) {
            //spdlog::info("WxBoxServer request profile");
            ResponseProfile();
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

#define WXBOT_STARTUP_FAILED()                                      \
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId()); \
    goto _Finish;

    // init
    if (!bot.Initialize()) {
        WXBOT_STARTUP_FAILED();
    }

    // execute hook
    if (!bot.HookWeChat()) {
        WXBOT_STARTUP_FAILED();
    }

    // resume all other wechat threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // start
    if (!bot.Startup()) {
        WXBOT_STARTUP_FAILED();
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

    // suspend all other threads
    wb_process::SuspendAllOtherThread(wb_process::GetCurrentProcessId(), wb_process::GetCurrentThreadId());

    // start wxbot
    std::thread(WxBotRoutine, std::move(duplicatedArgs)).detach();
}