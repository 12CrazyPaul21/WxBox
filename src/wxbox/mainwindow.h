#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QTranslator>
#include <QCloseEvent>
#include <QAtomicInteger>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#include "app_log.hpp"
#include "app_config.hpp"
#include "wxbox_server.hpp"

#include "about.h"

namespace Ui {
    class MainWindow;
}

using wxbox::WxBoxServerStatus;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    //
    // Constructor
    //

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    //
    // Events
    //

    virtual void changeEvent(QEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;

    //
    // Public Methods
    //

    std::vector<std::pair<std::string, std::string>> i18ns();
    void                                             changeLanguage(const std::string& language);
    void                                             startWxBoxServer();
    void                                             stopWxBoxServer();

  private:
    //
    // Private Methods
    //

    void ignoreForClose();
    void readyForClose(const bool canCloseWhenZero = true);

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
    void InitWxBox();
    void DeinitWxBox();
    void OpenAboutDialog();
    void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);
    void WxBoxServerEvent(wxbox::WxBoxMessage message);
    void triggerTest();

  private:
    bool                    init;
    bool                    wantToClose;
    QAtomicInteger<int32_t> readyForCloseCounter;

    Ui::MainWindow*          ui;
    AppConfig&               config;
    QTranslator              translator;
    AboutWxBoxDialog         aboutDialog;
    wxbox::WxBoxServerWorker worker;
};

#endif  // __MAINWINDOW_H
