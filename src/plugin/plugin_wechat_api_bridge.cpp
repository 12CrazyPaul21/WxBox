#include <plugin/plugin.h>

//
// Global Variables
//

static wb_plugin_wechat::FetchProfileBridge           fnFetchProfileBridge           = nullptr;
static wb_plugin_wechat::GetAllContactsBridge         fnGetAllContactsBridge         = nullptr;
static wb_plugin_wechat::GetContactWithWxNumberBridge fnGetContactWithWxNumberBridge = nullptr;
static wb_plugin_wechat::GetContactWithWxidBridge     fnGetContactWithWxidBridge     = nullptr;
static wb_plugin_wechat::WxNumberToWxidBridge         fnWxNumberToWxidBridge         = nullptr;
static wb_plugin_wechat::WxidToWxNumberBridge         fnWxidToWxNumberBridge         = nullptr;

//
// wechat api bridge register
//

void wxbox::plugin::wechat::RegisterFetchProfileBridge(FetchProfileBridge bridge)
{
    fnFetchProfileBridge = bridge;
}

void wxbox::plugin::wechat::RegisterGetAllContactsBridge(GetAllContactsBridge bridge)
{
    fnGetAllContactsBridge = bridge;
}

void wxbox::plugin::wechat::RegisterGetContactWithWxNumberBridge(GetContactWithWxNumberBridge bridge)
{
    fnGetContactWithWxNumberBridge = bridge;
}

void wxbox::plugin::wechat::RegisterGetContactWithWxidBridge(GetContactWithWxidBridge bridge)
{
    fnGetContactWithWxidBridge = bridge;
}

void wxbox::plugin::wechat::RegisterWxNumberToWxidBridge(WxNumberToWxidBridge bridge)
{
    fnWxNumberToWxidBridge = bridge;
}

void wxbox::plugin::wechat::RegisterWxidToWxNumberBridge(WxidToWxNumberBridge bridge)
{
    fnWxidToWxNumberBridge = bridge;
}

void wxbox::plugin::wechat::UnRegisterBridge()
{
    fnFetchProfileBridge           = nullptr;
    fnGetAllContactsBridge         = nullptr;
    fnGetContactWithWxNumberBridge = nullptr;
    fnGetContactWithWxidBridge     = nullptr;
    fnWxNumberToWxidBridge         = nullptr;
    fnWxidToWxNumberBridge         = nullptr;
}

//
// plugin wechat api bridge
//

bool wxbox::plugin::wechat::FetchProfile(wxbox::crack::wx::WeChatProfile& profile)
{
    if (!fnFetchProfileBridge) {
        return false;
    }
    return fnFetchProfileBridge(profile);
}

bool wxbox::plugin::wechat::GetAllContacts(std::vector<wxbox::crack::wx::WeChatContact>& contacts)
{
    if (!fnGetAllContactsBridge) {
        return false;
    }
    return fnGetAllContactsBridge(contacts);
}

bool wxbox::plugin::wechat::GetContactWithWxNumber(const std::string& wxnumber, wb_wx::WeChatContact& contact)
{
    if (!fnGetContactWithWxNumberBridge) {
        return false;
    }
    return fnGetContactWithWxNumberBridge(wxnumber, contact);
}

bool wxbox::plugin::wechat::GetContactWithWxid(const std::string& wxid, wb_wx::WeChatContact& contact)
{
    if (!fnGetContactWithWxidBridge) {
        return false;
    }
    return fnGetContactWithWxidBridge(wxid, contact);
}

std::string wxbox::plugin::wechat::WxNumberToWxid(const std::string& wxnumber)
{
    if (!fnWxNumberToWxidBridge) {
        return "";
    }
    return fnWxNumberToWxidBridge(wxnumber);
}

std::string wxbox::plugin::wechat::WxidToWxNumber(const std::string& wxid)
{
    if (!fnWxidToWxNumberBridge) {
        return "";
    }
    return fnWxidToWxNumberBridge(wxid);
}