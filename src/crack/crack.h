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
            char                                          wxbox_root[WXBOX_MAX_PATH];
            char                                          wxbot_root[WXBOX_MAX_PATH];
            char                                          plugins_root[WXBOX_MAX_PATH];
            char                                          wxbox_server_uri[WXBOX_MAX_PATH];
            int                                           wxbot_reconnect_interval;
            int                                           plugin_long_task_timeout;
            char                                          wechat_version[WXBOX_MAX_PATH];
            WxApis                                        wechat_apis;
            wxbox::crack::feature::WxDataStructSupplement wechat_datastructure_supplement;
        } WxBotEntryParameter, *PWxBotEntryParameter;

        PRAGMA(pack(pop))

        typedef struct _OpenWxWithMultiBoxingResult
        {
            wxbox::util::process::PID pid;
            void*                     pModuleBaseAddr;
            ucpulong_t                uModuleSize;
        } OpenWxWithMultiBoxingResult, *POpenWxWithMultiBoxingResult;

        using FnWeChatExitHandler   = std::function<void(void)>;
        using FnWeChatLogoutHandler = std::function<void(void)>;
        using FnWeChatLoginHandler  = std::function<void(void)>;

        //
        // Function
        //

        bool AttachWxProcess(wxbox::util::process::PID pid);
        void DeAttachWxProcess(wxbox::util::process::PID pid);

        bool GenerateWxApis(const wxbox::crack::feature::WxAPIHookPointVACollection& collection, WxApis& apis);
        bool VerifyWxApis(const WxApis& apis);

        bool OpenWxWithMultiBoxing(const wxbox::crack::wx::WeChatEnvironmentInfo& wxEnvInfo, wxbox::crack::feature::WxApiFeatures& wxApiFeatures, POpenWxWithMultiBoxingResult pResult = nullptr, bool keepAttach = false);

        bool IsWxBotInjected(wxbox::util::process::PID pid);
        bool InjectWxBot(wxbox::util::process::PID pid, const WxBotEntryParameter& parameter);
        bool UnInjectWxBot(wxbox::util::process::PID pid);
        bool UnInjectWxBotBySelf();

        bool PreInterceptWeChatExit(const WxApis& wxApis);
        void RegisterWeChatExitHandler(FnWeChatExitHandler handler);
        void UnRegisterWeChatExitHandler();

        bool PreInterceptWeChatLogout(const WxApis& wxApis);
        void RegisterWeChatLogoutHandler(FnWeChatLogoutHandler handler);
        void UnRegisterWeChatLogoutHandler();

        bool PreInterceptWeChatLogin(const WxApis& wxApis);
        void RegisterWeChatLoginHandler(FnWeChatLoginHandler handler);
        void UnRegisterWeChatLoginHandler();

        //
        // wechat api
        //

        uint8_t* FetchWeChatGlobalProfileContext(const WxApis& wxApis);

        bool IsLoign(const WxApis& wxApis, const wxbox::crack::feature::WxDataStructSupplement& wxDataSturctsupplement);
        bool FetchProfile(const WxApis& wxApis, const wxbox::crack::feature::WxDataStructSupplement& wxDataSturctsupplement, wxbox::crack::wx::WeChatProfile& profile);

        bool Logout(const WxApis& wxApis, const wxbox::crack::feature::WxDataStructSupplement& wxDataSturctsupplement);

        uint8_t* FetchWeChatGlobalContactContextAddress(const WxApis& wxApis);
        uint8_t* FetchContactHeaderAddress(const WxApis& wxApis, const wxbox::crack::feature::WxDataStructSupplement& wxDataSturctsupplement);
        bool     InitWeChatContactItem(const WxApis& wxApis, uint8_t* contactItem);
        bool     DeinitWeChatContactItem(const WxApis& wxApis, uint8_t* contactItem);
        bool     CollectAllContact(const PWxBotEntryParameter args, std::vector<wxbox::crack::wx::WeChatContact>& contacts);
        bool     GetContactWithWxNumber(const std::string& wxnumber, const PWxBotEntryParameter args, wxbox::crack::wx::WeChatContact& contact);
        bool     GetContactWithWxid(const std::string& wxid, const PWxBotEntryParameter args, wxbox::crack::wx::WeChatContact& contact);
    }
}

#endif  // #ifndef __WXBOX_CRACK_H