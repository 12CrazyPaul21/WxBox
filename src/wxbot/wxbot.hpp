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

        //
        // Plugin API
        //

        std::string ExecutePluginScript(const std::string& statement);

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

        //
        // WxBoxClient & PluginVirtualMachine EventHandler
        //

        void WxBoxClientEventHandler(WxBotMessage message);
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