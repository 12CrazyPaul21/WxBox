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
        WxBot(std::unique_ptr<wb_crack::WxBotEntryParameter>&& args)
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

        //
        // WeChat API Wrapper
        //

        bool GetContactWithWxNumber(const std::string& wxnumber, wb_wx::WeChatContact& contact);
        bool GetContactWithWxid(const std::string& wxid, wb_wx::WeChatContact& contact);

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
        bool WeChatSendMessageHandler(const wxbox::crack::wx::PWeChatWString wxid, const wxbox::crack::wx::PWeChatWString message, std::wstring& wxidSubstitute, std::wstring& messageSubstitute);

        //
        // Plugin Handler
        //

        void PluginExecuteResultEventHandler(const wb_plugin::PluginVirtualMachineExecuteResultEventPtr& resultEvent);
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
        std::unique_ptr<wb_crack::WxBotEntryParameter> args;
        std::vector<void*>                             hookPoints;
        std::mutex                                     mutex;
        std::atomic<bool>                              inited;
        std::atomic<bool>                              running;
        class WxBoxClient*                             client;
    };
}

WXBOT_PUBLIC_API void WxBotEntry(wxbox::crack::WxBotEntryParameter* args);

#endif  // #ifndef __WXBOT_H