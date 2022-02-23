#ifndef __WXBOX_APP_LOG_H
#define __WXBOX_APP_LOG_H

#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif

#if _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_TRACE_ON
#define SPDLOG_DEBUG_ON
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#undef SPDLOG_TRACE_ON
#undef SPDLOG_DEBUG_ON
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#define WXBOT_LOG_TRACE(FORMAT, ...)                               \
    if (spdlog::get(WXBOT_LOG_NAME)) {                             \
        spdlog::get(WXBOT_LOG_NAME)->trace(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOT_LOG_DEBUG(FORMAT, ...)                               \
    if (spdlog::get(WXBOT_LOG_NAME)) {                             \
        spdlog::get(WXBOT_LOG_NAME)->debug(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOT_LOG_INFO(FORMAT, ...)                               \
    if (spdlog::get(WXBOT_LOG_NAME)) {                            \
        spdlog::get(WXBOT_LOG_NAME)->info(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOT_LOG_WARNING(FORMAT, ...)                            \
    if (spdlog::get(WXBOT_LOG_NAME)) {                            \
        spdlog::get(WXBOT_LOG_NAME)->warn(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOT_LOG_ERROR(FORMAT, ...)                               \
    if (spdlog::get(WXBOT_LOG_NAME)) {                             \
        spdlog::get(WXBOT_LOG_NAME)->error(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOT_LOG_CRITICAL(FORMAT, ...)                               \
    if (spdlog::get(WXBOT_LOG_NAME)) {                                \
        spdlog::get(WXBOT_LOG_NAME)->critical(FORMAT, ##__VA_ARGS__); \
    }

#define WXBOX_LOG_INFO_AND_SHOW_MSG_BOX(VIEW, TITLE, FORMAT, ...) \
    spdlog::info(FORMAT, ##__VA_ARGS__);                          \
    xstyle::information(VIEW, Translate(TITLE), Translate(spdlog::fmt_lib::format(FORMAT, ##__VA_ARGS__).c_str()));

#define WXBOX_LOG_WARNING_AND_SHOW_MSG_BOX(VIEW, TITLE, FORMAT, ...) \
    spdlog::warn(FORMAT, ##__VA_ARGS__);                             \
    xstyle::warning(VIEW, Translate(TITLE), Translate(spdlog::fmt_lib::format(FORMAT, ##__VA_ARGS__).c_str()));

#define WXBOX_LOG_ERROR_AND_SHOW_MSG_BOX(VIEW, TITLE, FORMAT, ...) \
    spdlog::error(FORMAT, ##__VA_ARGS__);                          \
    xstyle::error(VIEW, Translate(TITLE), Translate(spdlog::fmt_lib::format(FORMAT, ##__VA_ARGS__).c_str()));

#define WXBOX_LOG_WECHAT_APIS(WECHAT_APIS)                                                                                                          \
    {                                                                                                                                               \
        spdlog::info("CheckAppSingleton va : 0x{:08X}", WECHAT_APIS.CheckAppSingleton);                                                             \
        spdlog::info("FetchGlobalContactContextAddress va : 0x{:08X}", WECHAT_APIS.FetchGlobalContactContextAddress);                               \
        spdlog::info("InitWeChatContactItem va : 0x{:08X}", WECHAT_APIS.InitWeChatContactItem);                                                     \
        spdlog::info("DeinitWeChatContactItem va : 0x{:08X}", WECHAT_APIS.DeinitWeChatContactItem);                                                 \
        spdlog::info("FindAndDeepCopyWeChatContactItemWithWXIDWrapper va : 0x{:08X}", WECHAT_APIS.FindAndDeepCopyWeChatContactItemWithWXIDWrapper); \
        spdlog::info("FetchGlobalProfileContext va : 0x{:08X}", WECHAT_APIS.FetchGlobalProfileContext);                                             \
        spdlog::info("HandleRawMessages va : 0x{:08X}", WECHAT_APIS.HandleRawMessages);                                                             \
        spdlog::info("HandleReceivedMessages va : 0x{:08X}", WECHAT_APIS.HandleReceivedMessages);                                                   \
        spdlog::info("WXSendTextMessage va : 0x{:08X}", WECHAT_APIS.WXSendTextMessage);                                                             \
        spdlog::info("FetchGlobalSendMessageContext va : 0x{:08X}", WECHAT_APIS.FetchGlobalSendMessageContext);                                     \
        spdlog::info("WXSendFileMessage va : 0x{:08X}", WECHAT_APIS.WXSendFileMessage);                                                             \
        spdlog::info("CloseLoginWnd va : 0x{:08X}", WECHAT_APIS.CloseLoginWnd);                                                                     \
        spdlog::info("LogoutAndExitWeChat va : 0x{:08X}", WECHAT_APIS.LogoutAndExitWeChat);                                                         \
    }

#define WXBOX_LOG_WECHAT_DATASTRUCTURE_SUPPLEMENT(WECHAT_DATASTRUCTURE_SUPPLEMENT)                                                          \
    {                                                                                                                                       \
        spdlog::info("WeChat Profile NickName item offset : 0x{:08X}", WECHAT_DATASTRUCTURE_SUPPLEMENT.profileItemOffset.NickName);         \
        spdlog::info("WeChat Profile WeChatNumber item offset : 0x{:08X}", WECHAT_DATASTRUCTURE_SUPPLEMENT.profileItemOffset.WeChatNumber); \
        spdlog::info("WeChat Profile Wxid item offset : 0x{:08X}", WECHAT_DATASTRUCTURE_SUPPLEMENT.profileItemOffset.Wxid);                 \
    }

#endif  // #ifndef __WXBOX_APP_LOG_H