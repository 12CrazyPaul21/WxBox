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

  private:
    //
    // WeChat Status
    //

    void UpdateWeChatStatus();

    //
    // WxBoxServer Wrapper Request Methods
    //

    void RequestProfile(wb_process::PID clientPID);

    //
    // WxBoxServer Response Handler
    //

    void ProfileResponseHandler(wb_process::PID clientPID, wxbox::ProfileResponse* response);

  signals:
    void PushMessageAsync(wxbox::WxBoxMessage message);

  public slots:

    //
    // WxBoxServer status changed and event handler
    //

    void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);
    void WxBoxServerEvent(wxbox::WxBoxMessage message);

  private:
    MainWindow* view;
    AppConfig&  config;

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    wb_feature::WxApiFeatures    wxApiFeatures;
    wxbox::WxBoxServer*          server;
    wxbox::WxBoxServerWorker     worker;
    int                          statusMonitorTimerId;
};

#endif  // #ifndef __WXBOX_CONTROLLER_H