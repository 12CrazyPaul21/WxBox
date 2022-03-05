#ifndef __WXBOX_CONTROLLER_ERROR_H
#define __WXBOX_CONTROLLER_ERROR_H

enum class WBCErrorCode
{
    WBC_NO_ERROR = 0,
    OPEN_WXBOX_SERVER_FAILED,
    INVALID_WECHAT_ENV_INFO,
    OPEN_WECHAT_FAILED,
    GET_WECHAT_PROCESS_HANDLE_FAILED,
    WECHAT_API_HOOK_POINT_INVALID,
    INJECT_WXBOT_MODULE_FAILED,
    RESOLVE_WECHAT_PROCESS_ENV_INFO_FAILED,
    WECHAT_PROCESS_IS_ALREADY_INJECT,
    WECHAT_PROCESS_IS_NOT_INJECTED,
    WECHAT_PROCESS_WXBOT_MODULE_NOT_CONNECTED,
};

const char* WXBOX_CONTROLLER_ERROR_MESSAGES[] = {
    "",
    "Start WxBoxServer failed, close application now...",
    "Invalid WeChat Environment Info",
    "Open WeChat failed",
    "Get WeChat process handle failed",
    "WeChat api hook point invalid",
    "Inject WxBot module invalid",
    "Resolve WeChat process environment info failed",
    "WeChat process is already inject",
    "WeChat process is not injected",
    "WeChat process's WxBot module not connected",
};

constexpr int WXBOX_CONTROLLER_ERROR_MESSAGES_COUNT = sizeof(WXBOX_CONTROLLER_ERROR_MESSAGES) / sizeof(char*);

#define WBC_SUCCESS(WBC_ERROR_CODE) (WBC_ERROR_CODE == WBCErrorCode::WBC_NO_ERROR)
#define WBC_FAILED(WBC_ERROR_CODE) (WBC_ERROR_CODE != WBCErrorCode::WBC_NO_ERROR)
#define WBC_MESSAGE(WBC_ERROR_CODE) (((int)WBC_ERROR_CODE < 0 || (int)WBC_ERROR_CODE >= WXBOX_CONTROLLER_ERROR_MESSAGES_COUNT) ? WXBOX_CONTROLLER_ERROR_MESSAGES[0] : WXBOX_CONTROLLER_ERROR_MESSAGES[(int)WBC_ERROR_CODE])
#define WBC_TRANSMESSAGE(WBC_ERROR_CODE) (Translate(WBC_MESSAGE(WBC_ERROR_CODE)))

#endif  // #ifndef __WXBOX_CONTROLLER_ERROR_H