#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <QApplication>
#include <QOperatingSystemVersion>
#include <QClipboard>
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
#include <QTextEdit>
#include <QToolBar>

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
#include <contact_list_dialog.h>
#include <setting_dialog.h>
#include <wxbox_client_status_model.h>

#define WXBOX_RESTART_STATUS_CODE 0x4325145

#define SHOW_SPLASH_SCREEN()                                                   \
    QSplashScreen* splash = new QSplashScreen(QPixmap(":/splash_screen.png")); \
    splash->setStyleSheet(R"(font: normal bold 14px "Microsoft YaHei";)");     \
    splash->show();
#define SPLASH_MESSAGE(MESSAGE) splash->showMessage(xstyle_manager.Translate("SplashScreen", MESSAGE), Qt::AlignHCenter | Qt::AlignBottom, Qt::lightGray);
#define SPLASH_FINISH(PWIDGET)   \
    {                            \
        splash->finish(&window); \
        delete splash;           \
        splash = nullptr;        \
    }

#define WXBOX_INFORMATION_YES_NO(MESSAGE) (xstyle::information(this, "", MESSAGE, XStyleMessageBoxButtonType::YesNo) == XStyleMessageBoxButton::Yes)

#ifdef WXBOX_XSTYLE_QUICK
#define XSTYLE_WINDOW_CLASS XStyleQuickWindow
#define WXBOX_QUICK_UI_URL "qrc:/wxbox/wxbox_quick_ui.qml"
#else
#define XSTYLE_WINDOW_CLASS XStyleWindow
#endif

#define DEFINE_CONTROLLER_CONFIG_MODIFIER(METHOD_NAME, TYPE_NAME, GETTER, SETTER) \
    inline void METHOD_NAME(const TYPE_NAME& v)                                   \
    {                                                                             \
        if (GETTER() != v) {                                                      \
            SETTER(v);                                                            \
            controller.RequestChangeConfig();                                     \
        }                                                                         \
    }

#define DEFINE_CONTROLLER_CONFIG_ASSOCIATE_ACTION_MODIFIER(METHOD_NAME, TYPE_NAME, GETTER, SETTER, MASK, WARNING) \
    inline void METHOD_NAME(const TYPE_NAME& v)                                                                   \
    {                                                                                                             \
        if (GETTER() != v) {                                                                                      \
            if (v && strlen(WARNING)) {                                                                           \
                xstyle::warning(this, "", Translate(WARNING));                                                    \
            }                                                                                                     \
            SETTER(v);                                                                                            \
            SetActionEnabled(MASK, v);                                                                            \
            controller.RequestChangeConfig();                                                                     \
        }                                                                                                         \
    }

#define DEFINE_CONTROLLER_STRING_CONFIG_MODIFIER(METHOD_NAME, TYPE_NAME, GETTER, SETTER) \
    inline void METHOD_NAME(const TYPE_NAME& v)                                          \
    {                                                                                    \
        if (GETTER().compare(v)) {                                                       \
            SETTER(v);                                                                   \
            controller.RequestChangeConfig();                                            \
        }                                                                                \
    }

namespace Ui {
    class MainWindowBody;
}

//
// MainWindow
//

#define RegisterMainWindowProperty(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    DefineXStyleProperty(Type, PropertyName, MemberName, Suffix, DefaultValue)           \
        Q_PROPERTY(Type PropertyName READ Get##Suffix WRITE Set##Suffix DESIGNABLE true SCRIPTABLE true)

class MainWindow final : public XSTYLE_WINDOW_CLASS
{
    friend class WxBoxController;

    Q_OBJECT

  public:
    RegisterMainWindowProperty(QString, status_icons, statusIcons, StatusIcons, DEFAULT_WXBOX_CLIENT_MODEL_STATUS_ICON_URLS);
    RegisterMainWindowProperty(QString, login_status_icons, loginStatusIcons, LoginStatusIcons, DEFAULT_WXBOX_CLIENT_MODEL_LOGIN_STATUS_ICON_URLS);

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    bool CheckSystemVersionSupported();
    void ModifySetting(int tabIndex = 0);
    void RequestWeChatInstallationPathSetting();
    void UpdateWeChatFeatures();
    void SettingChanged(const QString& name, const QVariant& newValue);
    void AppendExecuteCommandResult(const QString& result);
    void ClearCommandResultScreen();
    void ChangePluginCommandMaxHistoryLine(int maxLine);
    void SetActionEnabled(int mask, bool enabled);
    void RetranslateToolBar(int mask = 7);

    bool InitWxBox(QSplashScreen* splash);
    bool DeinitWxBox();

    virtual void OnBeginMission() Q_DECL_OVERRIDE;
    virtual void OnCloseMission() Q_DECL_OVERRIDE;

  private:
    virtual bool eventFilter(QObject* obj, QEvent* e) Q_DECL_OVERRIDE;

    virtual bool BeforeClose() Q_DECL_OVERRIDE
    {
        return DeinitWxBox();
    }

    virtual void MinimizedToTray() Q_DECL_OVERRIDE
    {
        if (!config["wxbox/already_show_minimized_to_tray_tips"_conf].safe_as<bool>()) {
            this->appTray.showMessage("", Translate("WxBox is Minimized To Tray"), QSystemTrayIcon::NoIcon, 3000);
            config["wxbox/already_show_minimized_to_tray_tips"_conf] = true;
        }
    }

    virtual void RetranslateUi() Q_DECL_OVERRIDE
    {
        XSTYLE_WINDOW_CLASS::RetranslateUi();
        auto language = xstyle_manager.CurrentLanguage().toStdString();
        config.change_language(language);
        wb_coredump::ChangeDumperLanguage(language);

        //
        // retranslate ui text
        //

        wxStatusModel.model().setHorizontalHeaderLabels(TranslateStringList(WxBoxClientStatusHeader));
        wxStatusModel.resize(false);

        // toolbar's actions
        RetranslateToolBar();
    }

    virtual void AfterThemeChanged(const QString& themeName) Q_DECL_OVERRIDE
    {
        auto theme = themeName.toStdString();
        config.change_theme(theme);
        wb_coredump::ChangeTheme(theme);

        //
        // apply widget's theme
        //

        wxStatusModel.applyTheme(statusIcons, loginStatusIcons);
    }

    inline void ChangeDumpPrefix(const QString& prefix)
    {
        auto newPrefix = prefix.toStdString();
        config.change_coredump_prefix(newPrefix);
        wb_coredump::ChangeDumpePrefix(newPrefix);
    }

    void SetWindowLoadingIconType(WindowLoadingIconType type)
    {
        XSTYLE_WINDOW_CLASS::SetWindowLoadingIconType(type);
        config.change_loading_icon_type((int)type);
    }

    void SetUseLoadingIconAnimationCache(bool useCache)
    {
        XSTYLE_WINDOW_CLASS::SetUseLoadingIconAnimationCache(useCache);
        config.change_loading_icon_animation_use_cache(useCache);
    }

    virtual void TurnCloseIsMinimizeToTray(bool toTray) Q_DECL_OVERRIDE
    {
        XSTYLE_WINDOW_CLASS::TurnCloseIsMinimizeToTray(toTray);
        if (config.close_is_minimize_to_tray() != toTray) {
            config.change_close_is_minimize_to_tray(toTray);
            if (!toTray && isHidden()) {
                show();
            }
            config["wxbox/already_show_minimized_to_tray_tips"_conf] = false;
        }
        qApp->setQuitOnLastWindowClosed(!toTray);
    }

    virtual void TurnAlwaysTopMost(bool enabled) Q_DECL_OVERRIDE
    {
        XSTYLE_WINDOW_CLASS::TurnAlwaysTopMost(enabled);
        if (config.always_top_most() != enabled) {
            config.change_always_top_most(enabled);
        }
    }

    DEFINE_CONTROLLER_CONFIG_ASSOCIATE_ACTION_MODIFIER(TurnAvoidRevokeMessage, bool, config.wechat_avoid_revoke_message, config.change_wechat_avoid_revoke_message, 0x1, "")
    DEFINE_CONTROLLER_CONFIG_ASSOCIATE_ACTION_MODIFIER(TurnEnableRawMessageHook, bool, config.wechat_enable_raw_message_hook, config.change_wechat_enable_raw_message_hook, 0x2, "If Enable Raw Message Hook, Maybe Cause WeChat Stucked")
    DEFINE_CONTROLLER_CONFIG_ASSOCIATE_ACTION_MODIFIER(TurnEnableSendTextMessageHook, bool, config.wechat_enable_send_text_message_hook, config.change_wechat_enable_send_text_message_hook, 0x4, "")
    DEFINE_CONTROLLER_CONFIG_MODIFIER(ChangeWxBoxClientReconnectInterval, int, config.wxbox_client_reconnect_interval, config.change_wxbox_client_reconnect_interval)
    DEFINE_CONTROLLER_CONFIG_MODIFIER(ChangePluginLongTaskTimeout, int, config.plugin_long_task_timeout, config.change_plugin_long_task_timeout)
    DEFINE_CONTROLLER_STRING_CONFIG_MODIFIER(ChangeWxBoxServerURI, std::string, config.wxbox_server_uri, config.change_wxbox_server_uri)

    virtual void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE
    {
        if (event->timerId() == controller.statusMonitorTimerId) {
            controller.UpdateWeChatStatus();
        }
    }

    void InitAppMenu();
    void InitAppTray();
    void InitWidget();
    void RegisterWidgetEventHandler();

  private:
    Ui::MainWindowBody*    ui;
    QToolBar*              toolbar;
    AboutWxBoxDialog       aboutDialog;
    DownloadDialog         downloadDialog;
    WxBoxSettingDialog     settingDialog;
    ContactListDialog      contactListDialog;
    XStyleMenu             appMenu;
    QSystemTrayIcon        appTray;
    XStyleMenu             clientItemContextMenu;
    WxBoxClientStatusModel wxStatusModel;

    AppConfig&      config;
    WxBoxController controller;
};

#endif  // __MAINWINDOW_H
