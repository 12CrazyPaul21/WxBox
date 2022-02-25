#include <wxbot.hpp>

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

void wxbot::WxBot::WeChatRawMessageHandler(wb_wx::WeChatMessageType type, wb_wx::PWeChatMessage message)
{
    WXBOX_UNREF(type);

    if (!message) {
        return;
    }
}

void wxbot::WxBot::WeChatPreReceivedMessageHandler(wb_wx::PWeChatMessage message)
{
    WXBOX_UNREF(message);
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

bool wxbot::WxBot::WeChatSendMessageHandler(const wxbox::crack::wx::PWeChatWString wxid, const wxbox::crack::wx::PWeChatWString message, std::wstring& wxidSubstitute, std::wstring& messageSubstitute)
{
    WXBOX_UNREF(wxidSubstitute);
    WXBOX_UNREF(messageSubstitute);

    if (!wxid || !message) {
        return false;
    }

    return false;
}