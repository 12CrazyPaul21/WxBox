#ifndef __WXBOT_H
#define __WXBOT_H

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_WXBOT
#define WXBOT_PUBLIC __declspec(dllexport)
#else
#define WXBOT_PUBLIC __declspec(dllimport)
#endif
#else
#ifdef BUILDING_WXBOT
#define WXBOT_PUBLIC __attribute__((visibility("default")))
#else
#define WXBOT_PUBLIC
#endif
#endif

#ifdef __cplusplus
#define WXBOT_PUBLIC_API extern "C" WXBOT_PUBLIC
#else
#define WXBOT_PUBLIC_API WXBOT_PUBLIC
#endif

#include <wxbox_client.hpp>
#include <plugin/plugin.h>

namespace wxbot {
    class WxBot
    {
      public:
        WxBot(wb_crack::WxBotEntryParameterPtr&& args)
          : args(std::move(args))
          , inited(false)
          , running(false)
          , client(nullptr)
        {
        }

        ~WxBot()
        {
            if (inited) {
                Stop();
                Wait();
                Shutdown();
            }
        }

        bool Initialize();
        bool Startup();
        void Wait();
        void Stop();
        void Shutdown();

        bool HookWeChat();
        void UnHookWeChat();

        void log(wxbox::WxBotLogLevel level, const char* format, ...);

        //
        // WeChat API Wrapper
        //

        std::string WxNumberToWxid(const std::string& wxnumber);
        std::string WxidToWxNumber(const std::string& wxid);

        bool SendTextMessageToContact(const std::string& wxid, const std::string& message);
        bool SendTextMessageToContactWithWxNumber(const std::string& wxnumber, const std::string& message);
        bool SendPictureToContact(const std::string& wxid, const std::string& imgPath);
        bool SendPictureToContactWithWxNumber(const std::string& wxnumber, const std::string& imgPath);
        bool SendFileToContact(const std::string& wxid, const std::string& filePath);
        bool SendFileToContactWithWxNumber(const std::string& wxnumber, const std::string& filePath);

        bool SendTextMessageToFileHelper(const std::string& message);
        bool SendPictureToFileHelper(const std::string& imgPath);
        bool SendFileToFileHelper(const std::string& filePath);

        bool SendTextMessageToChatroom(const std::string& roomWxid, const std::string& message);
        bool SendTextMessageToChatroomWithNotifyList(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists, const std::string& message);
        bool NotifyChatroomContacts(const std::string& roomWxid, const std::vector<std::string>& notifyWxidLists);
        bool NotifyAllChatroomContact(const std::string& roomWxid);
        bool NotifyAllChatroomContactWithTextMessage(const std::string& roomWxid, const std::string& message);
        bool SendPictureToChatroom(const std::string& roomWxid, const std::string& imgPath);
        bool SendFileToChatroom(const std::string& roomWxid, const std::string& filePath);

        //
        // Plugin API
        //

        void ExecutePluginScript(const std::string& statement);
        void DispatchPluginResult(const std::string& result, bool fromFilehelper);
        void DispatchPluginErrorReport(const std::string& errorMsg, bool fromFilehelper);
        void DispatchPluginReceiveRawWeChatMessage(wb_wx::WeChatMessageType type, wb_wx::PWeChatMessage message, bool sync = true);
        void DispatchPluginReceiveWeChatMessage(wb_wx::PWeChatMessage message, bool sync = false);
        void DispatchPluginReceiveTextWeChatMessage(wb_wx::PWeChatMessage message, bool sync = false);
        void DispatchPluginSendTextWeChatMessage(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message, bool sync = true);
        void DispatchPluginLoginWeChatMessage(bool sync = false);
        void DispatchPluginLogoutWeChatMessage(bool sync = false);
        void DispatchPluginExitWeChatMessage(bool sync = false);

      private:
        void PreHookWeChat();
        void ReleasePreHookWeChatHookPoint();
        void RegisterInterceptHanlders();
        void UnRegisterInterceptHanlders();
        void ExecuteHookWeChat(bool hook = true);

      private:
        //
        // WeChat Intercept Handler
        //

        void WeChatExitHandler();
        void WeChatLogoutHandler();
        void WeChatLoginHandler();
        void WeChatRawMessageHandler(wb_wx::WeChatMessageType type, wb_wx::PWeChatMessage message);
        void WeChatPreReceivedMessageHandler(wb_wx::PWeChatMessage message);
        void WeChatReceivedMessagesHandler(wb_wx::PWeChatMessageCollection messageCollection, ucpulong_t count, ucpulong_t presize);
        void WeChatSendTextMessageHandler(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message);

        //
        // Plugin Handler
        //

        void PluginExecuteResultEventHandler(const wb_plugin::PluginVirtualMachineExecuteResultEventPtr& resultEvent);
        void PluginSendMessageHandler(const wb_plugin::PluginSendWeChatMessagePtr& sendMessageArgs);
        void PluginToHostEventHandler(const wb_plugin::PluginVirtualMachinePluginToHostEventPtr& pluginToHostEvent);

        //
        // WxBoxClient & PluginVirtualMachine EventHandler
        //

        void WxBoxClientEventHandler(wxbot::WxBotMessage message);
        void WxBoxRequestOrResponseHandler(wxbot::WxBotMessage& message);
        void PluginVirtualMachineEventHandler(wb_plugin::PluginVirtualMachineEventPtr event);

        //
        // WxBoxClient Wrapper Response Methods
        //

        void ResponseInjectArgs();
        void ResponseProfile();
        void ResponseAllContact();
        void ResponseExecutePluginResult(const std::string& result);

      private:
        wb_crack::WxBotEntryParameterPtr args;
        std::vector<void*>               hookPoints;
        std::mutex                       mutex;
        std::atomic<bool>                inited;
        std::atomic<bool>                running;
        class WxBoxClient*               client;
    };
}

WXBOT_PUBLIC_API void WxBotEntry(wxbox::crack::WxBotEntryParameter* args);

#endif  // #ifndef __WXBOT_H