#include <wxbot.hpp>
#include <wxbox_client.hpp>

#include <spdlog/spdlog.h>

std::string ParseStatus(wxbot::WxBoxClientStatus status)
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

    bool WxBot::BuildWxBoxClient()
    {
        if (client) {
            return false;
        }

        client = new wxbot::WxBoxClient(wxbot::WxBoxClient::WxBoxServerURI());
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
