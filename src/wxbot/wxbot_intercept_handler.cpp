#include <wxbot.hpp>

//
// WeChat Intercept Handler
//

void wxbot::WxBot::WeChatExitHandler()
{
    DispatchPluginExitWeChatMessage(false);
    Stop();
}

void wxbot::WxBot::WeChatLogoutHandler()
{
    //
    // dispatch event to plugin
    //

    DispatchPluginLogoutWeChatMessage(false);

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
    //
    // dispatch event to plugin
    //

    DispatchPluginLoginWeChatMessage(false);

    //
    // report profile
    //

    ResponseProfile();
}

void wxbot::WxBot::WeChatRawMessageHandler(wb_wx::WeChatMessageType type, wb_wx::PWeChatMessage message)
{
    if (!message) {
        return;
    }

    if (IS_WECHAT_REVOKE_MESSAGE(type) && args->avoidRevokeMessage) {
        message->message_type = 0;
        message->message[0]   = L'0';
        return;
    }

    DispatchPluginReceiveRawWeChatMessage(type, message, true);
}

void wxbot::WxBot::WeChatPreReceivedMessageHandler(wb_wx::PWeChatMessage message)
{
    if (!message) {
        return;
    }

    IS_WECHAT_TEXT_MESSAGE(message->message_type) ? DispatchPluginReceiveTextWeChatMessage(message, false)
                                                  : DispatchPluginReceiveWeChatMessage(message, false);
}

void wxbot::WxBot::WeChatReceivedMessagesHandler(wb_wx::PWeChatMessageCollection messageCollection, ucpulong_t count, ucpulong_t presize)
{
    if (!messageCollection || !count || !presize) {
        return;
    }

    wb_wx::PWeChatMessage cursor = messageCollection->begin;

    for (ucpulong_t i = 0; i < count && cursor; i++) {
        WeChatPreReceivedMessageHandler(cursor);
        cursor = reinterpret_cast<wb_wx::PWeChatMessage>((((uint8_t*)(cursor)) + presize));
        if (cursor >= messageCollection->end) {
            break;
        }
    }
}

void wxbot::WxBot::WeChatSendTextMessageHandler(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message)
{
    if (!wxid || !message) {
        return;
    }

    DispatchPluginSendTextWeChatMessage(wxid, message, true);
}