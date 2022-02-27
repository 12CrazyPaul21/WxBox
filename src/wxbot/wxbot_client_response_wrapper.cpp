#include <wxbot.hpp>

//
// WxBoxClient Wrapper Response Methods
//

void wxbot::WxBot::ResponseInjectArgs()
{
    if (!args) {
        return;
    }

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::INJECT_ARGS_RESPONSE);

    auto injectArgsResponse = msg.u.wxBotControlPacket.mutable_injectargsresponse();
    injectArgsResponse->set_args(args.get(), sizeof(wb_crack::WxBotEntryParameter));

    client->PushMessageAsync(std::move(msg));
}

void wxbot::WxBot::ResponseProfile()
{
    // get wechat user profile
    wxbox::crack::wx::WeChatProfile profile;
    wb_crack::FetchProfile(profile);

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
    if (!wb_crack::CollectAllContact(contacts)) {
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

void wxbot::WxBot::ResponseExecutePluginResult(const std::string& result)
{
    if (result.empty()) {
        return;
    }

    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
    msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::EXECUTE_PLUGIN_SCRIPT_RESPONSE);

    auto executePluginScriptResponse = msg.u.wxBotControlPacket.mutable_executepluginscriptresponse();
    executePluginScriptResponse->set_result(result);

    client->PushMessageAsync(std::move(msg));
}