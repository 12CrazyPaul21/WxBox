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
#include <QSplashScreen>

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
#include <download_dialog.h>

#define SHOW_SPLASH_SCREEN()                                                   \
    QSplashScreen* splash = new QSplashScreen(QPixmap(":/splash_screen.png")); \
    splash->setStyleSheet(R"(font: normal bold 14px "Microsoft YaHei";)");     \
    splash->show();
#define SPLASH_MESSAGE(MESSAGE) splash->showMessage(xstyle_manager.Translate("SplashScreen", MESSAGE), Qt::AlignCenter | Qt::AlignBottom, Qt::lightGray);
#define SPLASH_FINISH(PWIDGET)   \
    {                            \
        splash->finish(&window); \
        delete splash;           \
        splash = nullptr;        \
    }

#ifdef WXBOX_XSTYLE_QUICK
#define XSTYLE_WINDOW_CLASS XStyleQuickWindow
#define WXBOX_QUICK_UI_URL "qrc:/wxbox/wxbox_quick_ui.qml"
#else
#define XSTYLE_WINDOW_CLASS XStyleWindow
#endif

namespace Ui {
    class MainWindowBody;
}

class MainWindow final : public XSTYLE_WINDOW_CLASS
{
    friend class WxBoxController;

    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    bool CheckSystemVersionSupported();
    void UpdateWeChatFeatures();

    bool InitWxBox(QSplashScreen* splash);
    bool DeinitWxBox();

  private:
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
    void InitWidget();

  private:
    Ui::MainWindowBody* ui;
    AboutWxBoxDialog    aboutDialog;
    DownloadDialog      downloadDialog;
    XStyleMenu          appMenu;
    QSystemTrayIcon     appTray;

    AppConfig&      config;
    WxBoxController controller;
};

#endif  // __MAINWINDOW_H
