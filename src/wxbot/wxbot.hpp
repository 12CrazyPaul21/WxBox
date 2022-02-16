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

namespace wxbot {

    class WxBoxClient;
    struct _WxBotMessage;

    class WXBOT_PUBLIC WxBot
    {
      public:
        WxBot();
        ~WxBot();

        bool Ping();
        bool BuildWxBoxClient();
        void DestroyWxBoxClient();
        bool StartWxBoxClient();
        void StopWxBoxClient();
        void Wait();
        void Shutdown();

      private:
        void WxBoxClientEvent(_WxBotMessage message);

        //
        // WxBoxClient Wrapper Response Methods
        //

        void ResponseProfile();

      private:
        class WxBoxClient* client;
    };
}

namespace wxbox {
    namespace crack {
        struct _WxBotEntryParameter;
    }
}

WXBOT_PUBLIC_API void WxBotEntry(wxbox::crack::_WxBotEntryParameter* args);

#endif  // #ifndef __WXBOT_H