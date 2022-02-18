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
        } WxApis, *PWxApis;

        typedef struct _WxBotEntryParameter
        {
            ucpulong_t wxbox_pid;
            char       wxbox_root[WXBOX_MAX_PATH];
            char       wxbot_root[WXBOX_MAX_PATH];
            char       wxbox_server_uri[WXBOX_MAX_PATH];
            WxApis     wechat_apis;
        } WxBotEntryParameter, *PWxBotEntryParameter;

        PRAGMA(pack(pop))

        typedef struct _OpenWxWithMultiBoxingResult
        {
            wxbox::util::process::PID pid;
            void*                     pModuleBaseAddr;
            ucpulong_t                uModuleSize;
        } OpenWxWithMultiBoxingResult, *POpenWxWithMultiBoxingResult;

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
    }
}

#endif  // #ifndef __WXBOX_CRACK_H