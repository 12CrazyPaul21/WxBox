#ifndef __WXBOX_CRACK_WX_H
#define __WXBOX_CRACK_WX_H

namespace wxbox {
    namespace crack {
        namespace wx {

            //
            // Macro
            //

#if WXBOX_IN_WINDOWS_OS
#define WX_WE_CHAT_EXE "WeChat.exe"
#define WX_WE_CHAT_CORE_MODULE "WeChatWin.dll"
#define WX_WE_CHAT_MAIN_WINDOW_CLASS_NAME "WeChatMainWndForPC"
#define WX_WE_CHAT_LOGIN_WINDOW_CLASS_NAME "WeChatLoginWndForPC"
#elif WXBOX_IN_MAC_OS
#define WX_WE_CHAT_EXE ""
#define WX_WE_CHAT_CORE_MODULE ""
#define WX_WE_CHAT_MAIN_WINDOW_CLASS_NAME ""
#define WX_WE_CHAT_LOGIN_WINDOW_CLASS_NAME ""
#endif

            //
            // Typedef
            //

            typedef struct _WeChatEnvironmentInfo
            {
                std::string installPath;
                std::string executeFileName;
                std::string executeAbsPath;
                std::string moduleFolderAbsPath;
                std::string coreModuleName;
                std::string coreModuleAbsPath;
                std::string version;

                _WeChatEnvironmentInfo() = default;

                SETUP_COPY_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath         = other.installPath;
                    executeFileName     = other.executeFileName;
                    executeAbsPath      = other.executeAbsPath;
                    moduleFolderAbsPath = other.moduleFolderAbsPath;
                    coreModuleName      = other.coreModuleName;
                    coreModuleAbsPath   = other.coreModuleAbsPath;
                    version             = other.version;
                }

                SETUP_MOVE_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath         = std::move(other.installPath);
                    executeFileName     = std::move(other.executeFileName);
                    executeAbsPath      = std::move(other.executeAbsPath);
                    moduleFolderAbsPath = std::move(other.moduleFolderAbsPath);
                    coreModuleName      = std::move(other.coreModuleName);
                    coreModuleAbsPath   = std::move(other.coreModuleAbsPath);
                    version             = std::move(other.version);
                }

            } WeChatEnvironmentInfo, *PWeChatEnvironmentInfo;

            typedef struct _WeChatProcessEnvironmentInfo
            {
                WeChatEnvironmentInfo     wxEnvInfo;
                wxbox::util::process::PID pid;
                void*                     pCoreModuleBaseAddr;
                ucpulong_t                uCoreModuleSize;

                _WeChatProcessEnvironmentInfo() = default;

                SETUP_COPY_METHOD(_WeChatProcessEnvironmentInfo, other)
                {
                    wxEnvInfo           = other.wxEnvInfo;
                    pid                 = other.pid;
                    pCoreModuleBaseAddr = other.pCoreModuleBaseAddr;
                    uCoreModuleSize     = other.uCoreModuleSize;
                }

                SETUP_MOVE_METHOD(_WeChatProcessEnvironmentInfo, other)
                {
                    wxEnvInfo           = std::move(other.wxEnvInfo);
                    pid                 = other.pid;
                    pCoreModuleBaseAddr = other.pCoreModuleBaseAddr;
                    uCoreModuleSize     = other.uCoreModuleSize;
                }
            } WeChatProcessEnvironmentInfo, *PWeChatProcessEnvironmentInfo;

            //
            // WeChat Profile
            //

            typedef struct _WeChatProfile
            {
                bool        logined;
                std::string nickname;
                std::string wxnumber;
                std::string wxid;

                _WeChatProfile()
                  : logined(false)
                {
                }

                _WeChatProfile(bool logined, const std::string& nickname, const std::string& wxnumber, const std::string& wxid)
                  : logined(logined)
                  , nickname(nickname)
                  , wxnumber(wxnumber)
                  , wxid(wxid)
                {
                }

                SETUP_COPY_METHOD(_WeChatProfile, other)
                {
                    logined  = other.logined;
                    nickname = other.nickname;
                    wxnumber = other.wxnumber;
                    wxid     = other.wxid;
                }

                SETUP_MOVE_METHOD(_WeChatProfile, other)
                {
                    logined  = other.logined;
                    nickname = std::move(other.nickname);
                    wxnumber = std::move(other.wxnumber);
                    wxid     = std::move(other.wxid);
                }
            } WeChatProfile, *PWeChatProfile;

            //
            // WeChat Contact
            //

            typedef struct _WeChatWString
            {
                wchar_t* str;
                uint32_t length;
                uint32_t length2;
                uint32_t unknown1;
                uint32_t unknown2;
            } WeChatWString, *PWeChatWString;

            typedef struct _WeChatContactItem_below_3_5_0_46
            {
                _WeChatContactItem_below_3_5_0_46* left;
                _WeChatContactItem_below_3_5_0_46* parent;
                _WeChatContactItem_below_3_5_0_46* right;
            } WeChatContactItem_below_3_5_0_46, *PWeChatContactItem_below_3_5_0_46;

            typedef struct _WeChatContactHeader_below_3_5_0_46
            {
                PWeChatContactItem_below_3_5_0_46 _left;
                PWeChatContactItem_below_3_5_0_46 begin;
                PWeChatContactItem_below_3_5_0_46 _right;
            } WeChatContactHeader_below_3_5_0_46, *PWeChatContactHeader_below_3_5_0_46;

            typedef struct _WeChatContactItem_above_3_5_0_46
            {
                _WeChatContactItem_above_3_5_0_46* next;
                _WeChatContactItem_above_3_5_0_46* prev;
            } WeChatContactItem_above_3_5_0_46, *PWeChatContactItem_above_3_5_0_46;

            typedef struct _WeChatContactHeader_above_3_5_0_46
            {
                PWeChatContactItem_above_3_5_0_46 begin;
                PWeChatContactItem_above_3_5_0_46 end;
            } WeChatContactHeader_above_3_5_0_46, *PWeChatContactHeader_above_3_5_0_46;

            // """ utf8 format """
            typedef struct _WeChatContact
            {
                bool        chatroom;
                std::string nickname;
                std::string wxnumber;
                std::string wxid;
                std::string remark;

                _WeChatContact()
                  : chatroom(false)
                {
                }

                _WeChatContact(bool chatroom, const std::string& nickname, const std::string& wxnumber, const std::string& wxid, const std::string& remark)
                  : chatroom(chatroom)
                  , nickname(nickname)
                  , wxnumber(wxnumber)
                  , wxid(wxid)
                  , remark(remark)
                {
                }

                SETUP_COPY_METHOD(_WeChatContact, other)
                {
                    chatroom = other.chatroom;
                    nickname = other.nickname;
                    wxnumber = other.wxnumber;
                    wxid     = other.wxid;
                    remark   = other.remark;
                }

                SETUP_MOVE_METHOD(_WeChatContact, other)
                {
                    chatroom = other.chatroom;
                    nickname = std::move(other.nickname);
                    wxnumber = std::move(other.wxnumber);
                    wxid     = std::move(other.wxid);
                    remark   = std::move(other.remark);
                }
            } WeChatContact, *PWeChatContact;

            //
            // Function
            //

            std::string GetWxInstallationPath();
            std::string GetWxModuleFolderPath(const std::string& installPath);

            std::string GetWxVersion(const std::string& moduleFolderPath);

            bool IsWxInstallationPathValid(const std::string& installPath);
            bool IsWxInstallationPathValid(const std::string& installPath, const std::string& moduleFolderPath);

            bool ResolveWxEnvInfo(WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const wxbox::util::process::PID& pid, WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const wxbox::util::process::PID& pid, WeChatProcessEnvironmentInfo& wxProcessEnvInfo);
            bool ResolveWxEnvInfo(const std::string& installPath, WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const std::string& installPath, const std::string& moduleFolderPath, WeChatEnvironmentInfo& wxEnvInfo);

            std::vector<wxbox::util::process::ProcessInfo> GetWeChatProcessList();
            std::vector<wxbox::util::process::PID>         GetWeChatProcessIdList();
            bool                                           CheckWeChatProcessValid(wxbox::util::process::PID pid);

            bool RaiseWeChatWindowToForeground(const wxbox::util::process::PID& pid);
        }
    }
}

#define WECHAT_CONTACT_ITEM_WXID_OFFSET 0x10
#define WECHAT_CONTACT_ITEM_WXNUMBER_OFFSET 0x24
#define WECHAT_CONTACT_ITEM_REMARK_OFFSET 0x58
#define WECHAT_CONTACT_ITEM_NICKNAME_OFFSET 0x6C

#define WECHAT_CONTACT_ITEM_WXID(CONTACT_ITEM) ((wb_wx::PWeChatWString)(reinterpret_cast<uint8_t*>(CONTACT_ITEM) + WECHAT_CONTACT_ITEM_WXID_OFFSET))
#define WECHAT_CONTACT_ITEM_WXNUMBER(CONTACT_ITEM) ((wb_wx::PWeChatWString)(reinterpret_cast<uint8_t*>(CONTACT_ITEM) + WECHAT_CONTACT_ITEM_WXNUMBER_OFFSET))
#define WECHAT_CONTACT_ITEM_REMARK(CONTACT_ITEM) ((wb_wx::PWeChatWString)(reinterpret_cast<uint8_t*>(CONTACT_ITEM) + WECHAT_CONTACT_ITEM_REMARK_OFFSET))
#define WECHAT_CONTACT_ITEM_NICKNAME(CONTACT_ITEM) ((wb_wx::PWeChatWString)(reinterpret_cast<uint8_t*>(CONTACT_ITEM) + WECHAT_CONTACT_ITEM_NICKNAME_OFFSET))

#endif  // #ifndef __WXBOX_CRACK_WX_H