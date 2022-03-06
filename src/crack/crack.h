#ifndef __WXBOX_CRACK_H
#define __WXBOX_CRACK_H

namespace wxbox {
    namespace crack {

        static constexpr auto WXBOT_ENTRY_METHOD_NAME = "WxBotEntry";

#if WXBOX_IN_WINDOWS_OS
        static constexpr auto WXBOT_MODULE_NAME = "wxbot.dll";
#else
        static constexpr auto WXBOT_MODULE_NAME = "wxbot.so";
#endif

#if WXBOX_CPU_IS_X86
        static constexpr ucpulong_t HOOK_OPCODE_LENGTH = 5;
#else
        static constexpr ucpulong_t HOOK_OPCODE_LENGTH = 14;
#endif

        //
        // Typedef
        //

        PRAGMA(pack(push, 1))

        typedef struct _WxApis
        {
            ucpulong_t CheckAppSingleton;
            ucpulong_t FetchGlobalContactContextAddress;
            ucpulong_t InitWeChatContactItem;
            ucpulong_t DeinitWeChatContactItem;
            ucpulong_t FindAndDeepCopyWeChatContactItemWithWXIDWrapper;
            ucpulong_t FetchGlobalProfileContext;
            ucpulong_t HandleRawMessages;
            ucpulong_t HandleReceivedMessages;
            ucpulong_t WXSendTextMessage;
            ucpulong_t FetchGlobalSendMessageContext;
            ucpulong_t WXSendFileMessage;
            ucpulong_t CloseLoginWnd;
            ucpulong_t LogoutAndExitWeChat;
            ucpulong_t Logouted;
            ucpulong_t LogoutedByMobile;
            ucpulong_t Logined;
            ucpulong_t WeChatEventProc;
        } WxApis, *PWxApis;

        typedef struct _WxBotEntryParameter
        {
            ucpulong_t                                    wxbox_pid;
            ucpulong_t                                    wxbot_pid;
            char                                          wxbox_root[WXBOX_MAX_PATH];
            char                                          wxbot_root[WXBOX_MAX_PATH];
            char                                          plugins_root[WXBOX_MAX_PATH];
            char                                          wxbox_server_uri[WXBOX_MAX_PATH];
            int                                           wxbot_reconnect_interval;
            int                                           plugin_long_task_timeout;
            char                                          wechat_version[WXBOX_MAX_PATH];
            char                                          wechat_install_path[WXBOX_MAX_PATH];
            char                                          wechat_coremodule_abspath[WXBOX_MAX_PATH];
            bool                                          avoidRevokeMessage;
            bool                                          enableRawMessageHook;
            bool                                          enableSendTextMessageHook;
            WxApis                                        wechat_apis;
            wxbox::crack::feature::WxDataStructSupplement wechat_datastructure_supplement;
        } WxBotEntryParameter, *PWxBotEntryParameter;

        using WxBotEntryParameterPtr = std::shared_ptr<WxBotEntryParameter>;

        PRAGMA(pack(pop))

        typedef struct _OpenWxWithMultiBoxingResult
        {
            wxbox::util::process::PID pid;
            void*                     pModuleBaseAddr;
            ucpulong_t                uModuleSize;
        } OpenWxWithMultiBoxingResult, *POpenWxWithMultiBoxingResult;

        using FnWeChatExitHandler             = std::function<void(void)>;
        using FnWeChatLogoutHandler           = std::function<void(void)>;
        using FnWeChatLoginHandler            = std::function<void(void)>;
        using FnWeChatRawMessageHandler       = std::function<void(wxbox::crack::wx::WeChatMessageType, wxbox::crack::wx::PWeChatMessage)>;
        using FnWeChatReceivedMessagesHandler = std::function<void(wxbox::crack::wx::PWeChatMessageCollection, ucpulong_t count, ucpulong_t presize)>;
        using FnWeChatSendMessageHandler      = std::function<void(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message)>;

        //
        // Function
        //

        bool AttachWxProcess(wxbox::util::process::PID pid);
        void DeAttachWxProcess(wxbox::util::process::PID pid);

        bool GenerateWxApis(const wxbox::crack::feature::WxAPIHookPointVACollection& collection, WxApis& apis);
        bool VerifyWxApis(const WxApis& apis);
        bool IsFullFeaturesValid(const WxBotEntryParameter& parameter);

        bool OpenWxWithMultiBoxing(const wxbox::crack::wx::WeChatEnvironmentInfo& wxEnvInfo, wxbox::crack::feature::WxApiFeatures& wxApiFeatures, POpenWxWithMultiBoxingResult pResult = nullptr, bool keepAttach = false);

        bool IsWxBotInjected(wxbox::util::process::PID pid);
        bool InjectWxBot(wxbox::util::process::PID pid, const WxBotEntryParameter& parameter);
        bool UnInjectWxBot(wxbox::util::process::PID pid);
        bool UnInjectWxBotBySelf(std::time_t msOvertime, std::time_t msWatchDogCheckInterval, bool forced);

        bool PreInterceptWeChatExit(const WxApis& wxApis);
        void RegisterWeChatExitHandler(FnWeChatExitHandler handler);
        void UnRegisterWeChatExitHandler();

        bool PreInterceptWeChatLogout(const WxApis& wxApis);
        void RegisterWeChatLogoutHandler(FnWeChatLogoutHandler handler);
        void UnRegisterWeChatLogoutHandler();

        bool PreInterceptWeChatLogin(const WxApis& wxApis);
        void RegisterWeChatLoginHandler(FnWeChatLoginHandler handler);
        void UnRegisterWeChatLoginHandler();

        bool PreInterceptWeChatHandleRawMessage(const WxApis& wxApis);
        void RegisterWeChatRawMessageHandler(FnWeChatRawMessageHandler handler);
        void UnRegisterWeChatRawMessageHandler();

        bool PreInterceptWeChatHandleReceviedMessages(const WxApis& wxApis);
        void RegisterWeChatReceviedMessagesHandler(FnWeChatReceivedMessagesHandler handler);
        void UnRegisterWeChatReceviedMessagesHandler();

        bool PreInterceptWeChatSendTextMessage(const WxApis& wxApis);
        void RegisterWeChatSendTextMessageHandler(FnWeChatSendMessageHandler handler);
        void UnRegisterWeChatSendTextMessageHandler();

        //
        // wechat api
        //

        void InitWeChatApiCrackEnvironment(WxBotEntryParameterPtr& args);
        void DeInitWeChatApiCrackEnvironment();

        uint8_t* FetchWeChatGlobalProfileContext();

        bool IsLoign();
        bool FetchProfile(wxbox::crack::wx::WeChatProfile& profile);

        bool Logout();

        uint8_t* FetchWeChatGlobalContactContextAddress();
        uint8_t* FetchContactHeaderAddress();
        bool     InitWeChatContactItem(uint8_t* contactItem);
        bool     DeinitWeChatContactItem(uint8_t* contactItem);
        bool     CollectAllContact(std::vector<wxbox::crack::wx::WeChatContact>& contacts);
        bool     GetContactWithNickName(const std::string& nickname, wxbox::crack::wx::WeChatContact& contact);
        bool     GetContactWithWxNumber(const std::string& wxnumber, wxbox::crack::wx::WeChatContact& contact);
        bool     GetContactWithWxid(const std::string& wxid, wxbox::crack::wx::WeChatContact& contact);

        bool SendTextMessage(const std::string& wxid, const std::string& message);
        bool SendTextMessageWithNotifyList(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists, const std::string& message);
        bool SendFile(const std::string& wxid, const std::string& filePath);

        bool SubstituteWeChatWString(wxbox::crack::wx::PWeChatWString original, const std::wstring& substitute);
    }
}

#define WECHAT_MESSAGE_FILTER(MESSAGE) (MESSAGE ? ((wxbox::crack::wx::PWeChatMessage)(MESSAGE))->message_type = 0 : 0)

#endif  // #ifndef __WXBOX_CRACK_H