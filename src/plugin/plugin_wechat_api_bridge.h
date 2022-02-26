#ifndef __WXBOX_PLUGIN_WECHAT_API_BRIDGE_H
#define __WXBOX_PLUGIN_WECHAT_API_BRIDGE_H

namespace wxbox {
    namespace plugin {
        namespace wechat {

            //
            // Typedef
            //

            using FetchProfileBridge           = std::function<bool(wxbox::crack::wx::WeChatProfile& profile)>;
            using GetAllContactsBridge         = std::function<bool(std::vector<wxbox::crack::wx::WeChatContact>& contacts)>;
            using GetContactWithWxNumberBridge = std::function<bool(const std::string& wxnumber, wb_wx::WeChatContact& contact)>;
            using GetContactWithWxidBridge     = std::function<bool(const std::string& wxid, wb_wx::WeChatContact& contact)>;
            using WxNumberToWxidBridge         = std::function<std::string(const std::string& wxnumber)>;
            using WxidToWxNumberBridge         = std::function<std::string(const std::string& wxid)>;

            //
            // wechat api bridge register
            //

            void RegisterFetchProfileBridge(FetchProfileBridge bridge);
            void RegisterGetAllContactsBridge(GetAllContactsBridge bridge);
            void RegisterGetContactWithWxNumberBridge(GetContactWithWxNumberBridge bridge);
            void RegisterGetContactWithWxidBridge(GetContactWithWxidBridge bridge);
            void RegisterWxNumberToWxidBridge(WxNumberToWxidBridge bridge);
            void RegisterWxidToWxNumberBridge(WxidToWxNumberBridge bridge);

            void UnRegisterBridge();

            //
            // wechat api bridge
            //

            bool        FetchProfile(wxbox::crack::wx::WeChatProfile& profile);
            bool        GetAllContacts(std::vector<wxbox::crack::wx::WeChatContact>& contacts);
            bool        GetContactWithWxNumber(const std::string& wxnumber, wb_wx::WeChatContact& contact);
            bool        GetContactWithWxid(const std::string& wxid, wb_wx::WeChatContact& contact);
            std::string WxNumberToWxid(const std::string& wxnumber);
            std::string WxidToWxNumber(const std::string& wxid);
        }
    }
}

#endif  // #ifndef __WXBOX_PLUGIN_WECHAT_API_BRIDGE_H