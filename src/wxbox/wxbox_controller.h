#ifndef __WXBOX_CONTROLLER_H
#define __WXBOX_CONTROLLER_H

#include <QObject>
#include <QOperatingSystemVersion>
#include <QMessageBox>

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

    bool CheckSystemVersionSupported();

    void StartWxBoxServer();
    void StopWxBoxServer();

    void LoadWeChatEnvironmentInfo();
    void ReloadFeatures();

  private:
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
    wxbox::WxBoxServerWorker     worker;
};

#endif  // #ifndef __WXBOX_CONTROLLER_H