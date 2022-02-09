#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <QApplication>
#include <QOperatingSystemVersion>
#include <QMainWindow>
#include <QStyleFactory>
#include <QTranslator>
#include <QCloseEvent>
#include <QAtomicInteger>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QTimer>
#include <QMenu>
#include <QDesktopServices>
#include <QSystemTrayIcon>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#include <xstyle/xstylewindow.h>
#include <xstyle/xstylemessagebox.h>
#include <xstyle/xstylemenu.h>

#ifdef WXBOX_XSTYLE_QUICK
#include <xstyle/quick/qtquick_xstylewindow.h>
#endif

#include <app_log.hpp>
#include <app_config.hpp>
#include <wxbox_controller.h>
#include <internal/downloader.hpp>

#include <about.h>

namespace Ui {
    class MainWindowBody;
}

#ifdef WXBOX_XSTYLE_QUICK
#define XSTYLE_WINDOW_CLASS XStyleQuickWindow
#define WXBOX_QUICK_UI_URL "qrc:/wxbox/wxbox_quick_ui.qml"
#else
#define XSTYLE_WINDOW_CLASS XStyleWindow
#endif

class WxBoxDownloader : public QObject
{
    Q_OBJECT

  public:
    explicit WxBoxDownloader(QObject* parent = nullptr)
      : QObject(parent)
    {
    }

    void cancel()
    {
        emit triggerCancel();
    }

  signals:
    void triggerCancel();
};

class MainWindow final : public XSTYLE_WINDOW_CLASS
{
    friend class WxBoxController;

    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    bool CheckSystemVersionSupported();

  private:
    virtual void CompleteShow() override
    {
        if (!inited) {
            inited = InitWxBox();
        }
    }

    virtual bool BeforeClose() override
    {
        return DeinitWxBox();
    }

    virtual void RetranslateUi() override
    {
        XSTYLE_WINDOW_CLASS::RetranslateUi();
        auto language              = xstyle_manager.CurrentLanguage().toStdString();
        config[WXBOX_LANGUAGE_KEY] = language;
        wb_coredump::ChangeDumperLanguage(language);
    }

    virtual void AfterThemeChanged(const QString& themeName) override
    {
        auto theme                   = themeName.toStdString();
        config[WXBOX_THEME_NAME_KEY] = theme;
        wb_coredump::ChangeTheme(theme);
    }

    virtual void TurnCloseIsMinimizeToTray(bool toTray) override
    {
        XSTYLE_WINDOW_CLASS::TurnCloseIsMinimizeToTray(toTray);
        if (config.close_is_minimize_to_tray() != toTray) {
            config[WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY_KEY] = toTray;
        }
        qApp->setQuitOnLastWindowClosed(!toTray);
    }

    void RegisterEvent();
    void InitAppMenu();
    void InitAppTray();
    bool InitWxBox();
    bool DeinitWxBox();

  private:
    Ui::MainWindowBody* ui;
    AppConfig&          config;

    AboutWxBoxDialog aboutDialog;
    XStyleMenu       appMenu;
    QSystemTrayIcon  appTray;

    WxBoxController                  controller;
    wxbox::internal::WxBoxDownloader downloader;
};

#endif  // __MAINWINDOW_H
