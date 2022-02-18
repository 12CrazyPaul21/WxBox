#include <wxbot.hpp>
#include <wxbox_client.hpp>

#include <spdlog/spdlog.h>

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

namespace wxbot {

    WxBot::WxBot()
      : client(nullptr)
    {
    }

    WxBot::~WxBot()
    {
        StopWxBoxClient();
        Wait();
        DestroyWxBoxClient();
    }

    bool WxBot::Ping()
    {
        if (!client) {
            return false;
        }

        return client->Ping();
    }

    bool WxBot::BuildWxBoxClient(const char* uri)
    {
        if (client) {
            return false;
        }

        client = new wxbot::WxBoxClient(uri);
        return !!client;
    }

    void WxBot::DestroyWxBoxClient()
    {
        if (client) {
            delete client;
            client = nullptr;
        }
    }

    bool WxBot::StartWxBoxClient()
    {
        if (!client) {
            return false;
        }

        client->RegisterWxBotCallback(std::bind(&WxBot::WxBoxClientEvent, this, std::placeholders::_1));
        return client->Start();
    }

    void WxBot::StopWxBoxClient()
    {
        if (!client) {
            return;
        }

        client->Stop();
    }

    void WxBot::Wait()
    {
        if (!client) {
            return;
        }

        client->Wait();
    }

    void WxBot::Shutdown()
    {
        google::protobuf::ShutdownProtobufLibrary();
    }

    void WxBot::WxBoxClientEvent(wxbot::WxBotMessage message)
    {
        if (message.type == wxbot::WxBotMessageType::WxBoxClientStatusChange) {
            spdlog::info("WxBoxClient status change, oldStatus<{}>, newStatus<{}>", ParseStatus(message.u.wxBoxClientStatus.oldStatus), ParseStatus(message.u.wxBoxClientStatus.newStatus));
        }
        else if (message.type == wxbot::WxBotMessageType::WxBoxRequestOrResponse) {
            if (message.u.wxBoxControlPacket.type() == wxbox::ControlPacketType::PROFILE_REQUEST) {
                spdlog::info("WxBoxServer request profile");
                ResponseProfile();
            }
        }
    }

    //
    // WxBoxClient Wrapper Response Methods
    //

    void WxBot::ResponseProfile()
    {
        wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
        msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::PROFILE_RESPONSE);
        msg.u.wxBotControlPacket.mutable_profileresponse()->set_wxid("<is a wxid for test>");
        if (client) {
            client->PushMessageAsync(std::move(msg));
        }
    }
}

static void WxBotRoutine(std::unique_ptr<wb_crack::WxBotEntryParameter> args)
{
    std::stringstream ss;
    ss << "wxbox server uri : " << args->wxbox_server_uri;
    MessageBoxA(NULL, ss.str().c_str(), "WxBot", MB_OK);

    // execute hook

    // start wxbox client
    wxbot::WxBot bot;
    bot.BuildWxBoxClient(args->wxbox_server_uri);
    bot.StartWxBoxClient();

    // resume all other wechat threads
    wb_process::ResumeAllThread(wb_process::GetCurrentProcessId());

    // wait for finish
    bot.Wait();
    bot.DestroyWxBoxClient();

    // execute unhook

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