#ifndef __WXBOX_CONTROLLER_H
#define __WXBOX_CONTROLLER_H

#include <QObject>
#include <QOperatingSystemVersion>
#include <QMessageBox>

#include <xstyle/xstyle.h>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#include <app_log.hpp>
#include <app_config.hpp>
#include <wxbox_server.hpp>
#include <wxbox_client_status_model.h>

using wxbox::WxBoxServerStatus;

class WxBoxController final : public QObject
{
    friend class MainWindow;

    Q_OBJECT

  public:
    explicit WxBoxController(MainWindow* view = nullptr);
    ~WxBoxController();

    inline QString Translate(const char* text)
    {
        return xstyle_manager.Translate("WxBoxController", text);
    }

    bool CheckSystemVersionSupported();

    void StartWxBoxServer();
    void StopWxBoxServer();

    void StartWeChatStatusMonitor();
    void StopWeChatStatusMonitor();
    void ChangeWeChatStatusMonitorInterval(int interval);

    void LoadWeChatEnvironmentInfo();
    bool RequireValidWeChatEnvironmentInfo();
    void ReloadFeatures();

    bool StartWeChatInstance();
    bool InjectWxBotModule(wb_process::PID pid);
    bool UnInjectWxBotModule(wb_process::PID pid);
    void RaiseClientWindowToForeground(wb_process::PID pid);
    void DisplayClientInjectArgs(wb_process::PID pid);

  private:
    //
    // WeChat Status
    //

    inline bool IsClientAlive(wb_process::PID pid) const noexcept
    {
        return server && server->IsClientAlive(pid);
    }

    inline WxBoxClientItemStatus GetClientStatus(wb_process::PID pid) const noexcept
    {
        WxBoxClientItemStatus status = WxBoxClientItemStatus::Independent;
        if (wb_crack::IsWxBotInjected(pid)) {
            status = IsClientAlive(pid) ? WxBoxClientItemStatus::Normal : WxBoxClientItemStatus::Injected;
        }
        return status;
    }

    void UpdateClientProfile(wb_process::PID pid, const wb_wx::WeChatProfile& profile);
    void UpdateClientStatus(wb_process::PID pid) noexcept;
    void UpdateWeChatStatus();

    //
    // WxBoxServer Wrapper Request Methods
    //

    void RequestInjectArgs(wb_process::PID clientPID);
    void RequestProfile(wb_process::PID clientPID);
    void RequstLogoutWeChat(wb_process::PID clientPID);
    void RequstAllContact(wb_process::PID clientPID);
    void RequestExecutePluginScript(wb_process::PID clientPID, const std::string& statement);

    //
    // WxBoxServer Response Handler
    //

    void InjectArgsResponseHandler(wb_process::PID clientPID, wxbox::InjectArgsResponse* response);
    void ProfileResponseHandler(wb_process::PID clientPID, wxbox::ProfileResponse* response);
    void AllContactResponseHandler(wb_process::PID clientPID, wxbox::AllContactResponse* response);
    void ExecutePluginScriptResponseHandler(wb_process::PID clientPID, wxbox::ExecutePluginScriptResponse* response);
    void WxBotLogRequestHandler(wb_process::PID clientPID, wxbox::LogRequest* logRequest);

  signals:
    void PushMessageAsync(wxbox::WxBoxMessage message);

  public slots:

    //
    // WxBoxServer status changed and event handler
    //

    void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);
    void WxBoxServerEvent(wxbox::WxBoxMessage message);
    void WxBotRequestOrResponseHandler(wxbox::WxBoxMessage& message);

  private:
    MainWindow* view;
    AppConfig&  config;

    wb_wx::WeChatEnvironmentInfo                                               wxEnvInfo;
    wb_feature::WxApiFeatures                                                  wxApiFeatures;
    wxbox::WxBoxServer*                                                        server;
    wxbox::WxBoxServerWorker                                                   worker;
    int                                                                        statusMonitorTimerId;
    int                                                                        statusMonitorInterval;
    time_t                                                                     lastUpdateStatusTimestamp;
    mutable std::unordered_map<wb_process::PID, wb_crack::WxBotEntryParameter> clientInjectArgs;
};

#endif  // #ifndef __WXBOX_CONTROLLER_H