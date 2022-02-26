#include <wxbot.hpp>

//
// WeChat API Wrapper
//

bool wxbot::WxBot::FetchProfile(wxbox::crack::wx::WeChatProfile& profile)
{
    return wb_crack::FetchProfile(args->wechat_apis, args->wechat_datastructure_supplement, profile);
}

bool wxbot::WxBot::GetAllContacts(std::vector<wxbox::crack::wx::WeChatContact>& contacts)
{
    return wb_crack::CollectAllContact(args.get(), contacts);
}

bool wxbot::WxBot::GetContactWithWxNumber(const std::string& wxnumber, wb_wx::WeChatContact& contact)
{
    if (!args) {
        return false;
    }

    return wb_crack::GetContactWithWxNumber(wxnumber, args.get(), contact);
}

bool wxbot::WxBot::GetContactWithWxid(const std::string& wxid, wb_wx::WeChatContact& contact)
{
    if (!args) {
        return false;
    }

    return wb_crack::GetContactWithWxid(wxid, args.get(), contact);
}

std::string wxbot::WxBot::WxNumberToWxid(const std::string& wxnumber)
{
    if (wxnumber.empty()) {
        return "";
    }

    wb_wx::WeChatContact contact;
    if (!GetContactWithWxNumber(wxnumber, contact)) {
        return "";
    }

    return contact.wxid;
}

std::string wxbot::WxBot::WxidToWxNumber(const std::string& wxid)
{
    if (wxid.empty()) {
        return "";
    }

    wb_wx::WeChatContact contact;
    if (!GetContactWithWxid(wxid, contact)) {
        return "";
    }

    return contact.wxnumber;
}

bool wxbot::WxBot::SendTextMessageToContact(const std::string& wxid, const std::string& message)
{
    return wb_crack::SendTextMessage(args.get(), wxid, message);
}

bool wxbot::WxBot::SendTextMessageToContactWithWxNumber(const std::string& wxnumber, const std::string& message)
{
    std::string wxid = WxNumberToWxid(wxnumber);
    if (wxid.empty()) {
        return false;
    }

    return SendTextMessageToContact(wxid, message);
}

bool wxbot::WxBot::SendPictureToContact(const std::string& wxid, const std::string& imgPath)
{
    return wb_crack::SendFile(args.get(), wxid, imgPath);
}

bool wxbot::WxBot::SendPictureToContactWithWxNumber(const std::string& wxnumber, const std::string& imgPath)
{
    std::string wxid = WxNumberToWxid(wxnumber);
    if (wxid.empty()) {
        return false;
    }

    return SendPictureToContact(wxid, imgPath);
}

bool wxbot::WxBot::SendFileToContact(const std::string& wxid, const std::string& filePath)
{
    return wb_crack::SendFile(args.get(), wxid, filePath);
}

bool wxbot::WxBot::SendFileToContactWithWxNumber(const std::string& wxnumber, const std::string& filePath)
{
    std::string wxid = WxNumberToWxid(wxnumber);
    if (wxid.empty()) {
        return false;
    }

    return SendFileToContact(wxid, filePath);
}

bool wxbot::WxBot::SendTextMessageToFileHelper(const std::string& message)
{
    return SendTextMessageToContact("filehelper", message);
}

bool wxbot::WxBot::SendPictureToFileHelper(const std::string& imgPath)
{
    return SendPictureToContact("filehelper", imgPath);
}

bool wxbot::WxBot::SendFileToFileHelper(const std::string& filePath)
{
    return SendFileToContact("filehelper", filePath);
}

bool wxbot::WxBot::SendTextMessageToChatroom(const std::string& roomWxid, const std::string& message)
{
    return wb_crack::SendTextMessage(args.get(), roomWxid, message);
}

bool wxbot::WxBot::SendTextMessageToChatroomWithNotifyList(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists, const std::string& message)
{
    return wb_crack::SendTextMessageWithNotifyList(args.get(), roomWxid, notifyWxidLists, message);
}

bool wxbot::WxBot::NotifyChatroomContacts(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists)
{
    return wb_crack::SendTextMessageWithNotifyList(args.get(), roomWxid, notifyWxidLists, "");
}

bool wxbot::WxBot::NotifyAllChatroomContact(const std::string& roomWxid)
{
    return wb_crack::SendTextMessageWithNotifyList(args.get(), roomWxid, {"notify@all"}, "");
}

bool wxbot::WxBot::NotifyAllChatroomContactWithTextMessage(const std::string& roomWxid, const std::string& message)
{
    return wb_crack::SendTextMessageWithNotifyList(args.get(), roomWxid, {"notify@all"}, message);
}

bool wxbot::WxBot::SendPictureToChatroom(const std::string& roomWxid, const std::string& imgPath)
{
    return wb_crack::SendFile(args.get(), roomWxid, imgPath);
}

bool wxbot::WxBot::SendFileToChatroom(const std::string& roomWxid, const std::string& filePath)
{
    return wb_crack::SendFile(args.get(), roomWxid, filePath);
}