#include "mainwindow.h"
#include "ui_mainwindow.h"

#define WXBOX_MAIN_WINDOW_NAME "MainWindow"
#define WXBOX_MAIN_WINDOW_TITLE "WxBox - " WXBOX_VERSION
#define WXBOX_ICON_URL ":/icon/app.ico"

MainWindow::MainWindow(QWidget* parent)
  : XSTYLE_WINDOW_CLASS(WXBOX_MAIN_WINDOW_NAME, parent, false)
  , ui(new Ui::MainWindowBody)
  , config(AppConfig::singleton())
  , aboutDialog(this)
  , downloadDialog(this)
  , settingDialog(this)
  , contactListDialog(this)
  , appMenu("AppMenu", this)
  , appTray(this)
  , clientItemContextMenu("ClientItemContextMenu", this)
  , wxStatusModel(this)
  , controller(this)
{
    // setup xstyle ui
    SetupXStyleUi(ui);
    SetWindowTitle(Translate(WXBOX_MAIN_WINDOW_TITLE));

#ifdef WXBOX_XSTYLE_QUICK
    // setup xstyle quick ui
    SetupXStyleQuick(ui->quickWidget, WXBOX_QUICK_UI_URL);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress && obj == ui->viewWeChatStatus->viewport()) {
        auto mouseEvent = static_cast<QMouseEvent*>(e);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            auto hitRow = ui->viewWeChatStatus->indexAt(mouseEvent->pos()).row();
            if (hitRow == -1) {
                ui->viewWeChatStatus->clearSelection();
                return true;
            }

            auto selectedRows = ui->viewWeChatStatus->selectionModel()->selectedRows();
            if (!selectedRows.empty() && selectedRows[0].row() == hitRow) {
                ui->viewWeChatStatus->clearSelection();
                return true;
            }
        }
    }

    return XSTYLE_WINDOW_CLASS::eventFilter(obj, e);
}

bool MainWindow::CheckSystemVersionSupported()
{
    if (controller.CheckSystemVersionSupported()) {
        return true;
    }

#if WXBOX_IN_WINDOWS_OS
    static QString msg = Translate("Only systems above Windows 7 are supported.");
#else
    static QString msg = Translate("Only systems above Mac OS Sierra are supported.");
#endif

    xstyle::information(nullptr, Translate("WxBox"), msg);
    return false;
}

void MainWindow::ModifySetting(int tabIndex)
{
    if (settingDialog.ModifySetting(tabIndex) && WXBOX_INFORMATION_YES_NO(Translate("Restart WxBox To Apply Setting Changed?"))) {
        //
        // trigger restart WxBox
        //

        quit();

        while (readyForCloseCounter) {
            QApplication::processEvents();
            QThread::msleep(10);
        }

        qApp->exit(WXBOX_RESTART_STATUS_CODE);
    }
}

void MainWindow::RequestWeChatInstallationPathSetting()
{
    settingDialog.ModifySetting(3);
}

void MainWindow::UpdateWeChatFeatures()
{
    downloadDialog.BeginMission();
    spdlog::info("update wechat features");

    // pulling feature list
    downloadDialog.SetWindowTitle(Translate("Update Feature Repository"));
    downloadDialog.SetStatus(Translate("Pulling Feature List"));
    auto result = downloadDialog.get(QUrl(config.features_list_url().c_str()));
    if (!std::get<0>(result)) {
        downloadDialog.CloseMission();
        xstyle::error(this, "", Translate("Pulling Feature List Failed"));
        return;
    }

    // parse feature list
    wb_feature::FeatureRepoList repoFeatureList;
    if (!wb_feature::ParseFeatureRepoList(std::get<1>(result), repoFeatureList)) {
        downloadDialog.CloseMission();
        xstyle::error(this, "", Translate("Feature List Invalid"));
        return;
    }

    // check for updates
    if (!config.feature_update_timestamp().compare(repoFeatureList.timestamp)) {
        downloadDialog.CloseMission();
        xstyle::information(this, "", Translate("Feature Repository is already up to date"));
        return;
    }

    // download features
    if (downloadDialog.download(config.features_path().c_str(), repoFeatureList.features) == FileDownloadStatus::Success) {
        config.update_feature_update_timestamp(repoFeatureList.timestamp);
        spdlog::info("update wechat features successful, the version timestamp : " + repoFeatureList.timestamp);
    }

    downloadDialog.CloseMission();

    // reload features
    controller.ReloadFeatures();
}

void MainWindow::SettingChanged(const QString& name, const QVariant& newValue)
{
#define BEGIN_SETTING_MODIFY_CHECK(SETTING_NAME, MODIFY_METHOD, VALUE) \
    if (!name.compare(SETTING_NAME)) {                                 \
        MODIFY_METHOD(VALUE);                                          \
    }

#define NEXT_SETTING_MODIFY_CHECK(SETTING_NAME, VALUE_TYPE, MODIFY_METHOD) \
    else if (!name.compare(SETTING_NAME))                                  \
    {                                                                      \
        MODIFY_METHOD(newValue.value<VALUE_TYPE>());                       \
    }

#define NEXT_SETTING_CONVERT_MODIFY_CHECK(SETTING_NAME, VALUE_TYPE, CONVERT, MODIFY_METHOD) \
    else if (!name.compare(SETTING_NAME))                                                   \
    {                                                                                       \
        MODIFY_METHOD(newValue.value<VALUE_TYPE>() CONVERT);                                \
    }

#define NEXT_ENUM_SETTING_MODIFY_CHECK(SETTING_NAME, VALUE_TYPE, ENUM_TYPE, MODIFY_METHOD) \
    else if (!name.compare(SETTING_NAME))                                                  \
    {                                                                                      \
        MODIFY_METHOD((ENUM_TYPE)newValue.value<VALUE_TYPE>());                            \
    }

#define NEXT_STD_STRING_SETTING_MODIFY_CHECK(SETTING_NAME, MODIFY_METHOD) \
    else if (!name.compare(SETTING_NAME))                                 \
    {                                                                     \
        MODIFY_METHOD(newValue.value<QString>().toStdString());           \
    }

    BEGIN_SETTING_MODIFY_CHECK("WxBoxServerPort", ChangeWxBoxServerURI, QString("localhost:%1").arg(newValue.value<QString>()).toStdString())
    NEXT_SETTING_MODIFY_CHECK("WxBoxClientReconnectInterval", int, ChangeWxBoxClientReconnectInterval)
    NEXT_SETTING_MODIFY_CHECK("CloseIsMinimizeToTray", bool, TurnCloseIsMinimizeToTray)
    NEXT_SETTING_MODIFY_CHECK("AlwaysTopMost", bool, TurnAlwaysTopMost)
    NEXT_SETTING_MODIFY_CHECK("LoadingIconAnimationUseCache", bool, SetUseLoadingIconAnimationCache)
    NEXT_ENUM_SETTING_MODIFY_CHECK("LoadingIconType", int, WindowLoadingIconType, SetWindowLoadingIconType)
    NEXT_SETTING_MODIFY_CHECK("Language", QString, xstyle_manager.ChangeLanguage)
    NEXT_SETTING_MODIFY_CHECK("Theme", QString, xstyle_manager.ChangeTheme)
    NEXT_SETTING_MODIFY_CHECK("CoreDumpPrefix", QString, ChangeDumpPrefix)
    NEXT_STD_STRING_SETTING_MODIFY_CHECK("LogBasename", config.change_log_name)
    NEXT_SETTING_MODIFY_CHECK("LogMaxRotatingFileCount", int, config.change_log_max_rotating_file_count)
    NEXT_SETTING_CONVERT_MODIFY_CHECK("LogMaxSingleFileSize", int, *1048576, config.change_log_max_single_file_size)
    NEXT_SETTING_MODIFY_CHECK("LogAutoFlushInterval", int, config.change_log_auto_flush_interval_sec)
    NEXT_SETTING_MODIFY_CHECK("PluginLongTaskTimeout", int, ChangePluginLongTaskTimeout)
    NEXT_SETTING_MODIFY_CHECK("PluginLogMaxLine", int, config.change_plugin_log_max_line)
    NEXT_SETTING_MODIFY_CHECK("PluginCommandMaxHistoryLine", int, ChangePluginCommandMaxHistoryLine)
    NEXT_SETTING_MODIFY_CHECK("PluginCommandMaxHistoryPersistenceLine", int, config.change_plugin_command_max_history_persistence_line)
    NEXT_STD_STRING_SETTING_MODIFY_CHECK("FeatureRepoRootURL", config.change_features_repo_root_url)
    NEXT_SETTING_MODIFY_CHECK("AvoidRevokeMessage", bool, TurnAvoidRevokeMessage)
    NEXT_SETTING_MODIFY_CHECK("EnableRawMessageHook", bool, TurnEnableRawMessageHook)
    NEXT_SETTING_MODIFY_CHECK("EnableSendTextMessageHook", bool, TurnEnableSendTextMessageHook)
    NEXT_STD_STRING_SETTING_MODIFY_CHECK("WeChatInstallationPath", controller.ChangeWeChatInstallationPath)
    NEXT_STD_STRING_SETTING_MODIFY_CHECK("WeChatCoreModulePath", controller.ChangeWeChatCoreModulePath)
    NEXT_SETTING_MODIFY_CHECK("ClientStatusUpdateInterval", int, controller.ChangeWeChatStatusMonitorInterval)
}

void MainWindow::AppendExecuteCommandResult(const QString& result)
{
    if (result.isEmpty()) {
        return;
    }

    if (ui->viewCommandExecuteLogger->document()->lineCount() > config.plugin_log_max_line()) {
        QTextCursor cursor = ui->viewCommandExecuteLogger->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 0);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar();
    }

    ui->viewCommandExecuteLogger->append(result);
    ui->viewCommandExecuteLogger->moveCursor(QTextCursor::End);
}

void MainWindow::ClearCommandResultScreen()
{
    ui->viewCommandExecuteLogger->clear();
}

void MainWindow::ChangePluginCommandMaxHistoryLine(int maxLine)
{
    if (config.plugin_command_max_history_line() != maxLine) {
        config.change_plugin_command_max_history_line(maxLine);
        ui->lineCommand->SetMaxHistoryLine(maxLine);
    }
}

void MainWindow::OnBeginMission()
{
    XSTYLE_WINDOW_CLASS::OnBeginMission();
    ui->btnStartWeChat->setEnabled(false);
}

void MainWindow::OnCloseMission()
{
    XSTYLE_WINDOW_CLASS::OnCloseMission();
    ui->btnStartWeChat->setEnabled(true);
}

void MainWindow::InitAppMenu()
{
    // application menu
    appMenu.pushAction("Setting", &settingDialog, std::bind(&MainWindow::ModifySetting, this, 0));
    appMenu.pushSeparator();
    appMenu.pushAction("Visit Repository", this, std::bind(&QDesktopServices::openUrl, QUrl(WXBOX_REPOSITORY_URL)));
    appMenu.pushAction("About WxBox", &aboutDialog, std::bind(&AboutWxBoxDialog::showApplicationModal, &aboutDialog));
    appMenu.pushSeparator();
    appMenu.pushAction("Exit WxBox", this, std::bind(&MainWindow::quit, this));

    // client item context menu
    clientItemContextMenu.pushAction("Inject");
    clientItemContextMenu.pushAction("UnInject");
    clientItemContextMenu.pushAction("Raise To Foreground");
    clientItemContextMenu.pushAction("Show Feature Info");
    clientItemContextMenu.pushSeparator();
    clientItemContextMenu.pushAction("Copy NickName");
    clientItemContextMenu.pushAction("Copy WxNumber");
    clientItemContextMenu.pushAction("Copy WXID");
    clientItemContextMenu.pushSeparator();
    clientItemContextMenu.pushAction("Refresh Profile");
    clientItemContextMenu.pushAction("All Contact");
    clientItemContextMenu.pushSeparator();
    clientItemContextMenu.pushAction("Logout");
}

void MainWindow::InitAppTray()
{
    appTray.setContextMenu(&appMenu);
    appTray.setToolTip(WXBOX_MAIN_WINDOW_TITLE);
    appTray.setIcon(QIcon(WXBOX_ICON_URL));
    appTray.show();

    connect(&appTray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::ActivationReason::DoubleClick) {
            show();
        }
    });
}

void MainWindow::InitWidget()
{
    //
    // wechat status table view
    //

    ui->viewWeChatStatus->setModel(&wxStatusModel.model());
    ui->viewWeChatStatus->verticalHeader()->hide();
    ui->viewWeChatStatus->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->viewWeChatStatus->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->viewWeChatStatus->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->viewWeChatStatus->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);

    // wechat status table model
    wxStatusModel.setContainer(ui->viewWeChatStatus);
    wxStatusModel.model().setHorizontalHeaderLabels(TranslateStringList(WxBoxClientStatusHeader));
    wxStatusModel.resize();

    // apply wechat status table view theme
    wxStatusModel.applyTheme(statusIcons, loginStatusIcons);

    //
    // wxbox plugin command line edit
    //

    ui->lineCommand->SetMaxHistoryLine(config.plugin_command_max_history_line());
    ui->lineCommand->LoadHistory(config.load_plugin_command_history());
    ui->lineCommand->setEnabled(false);

    //
    // setting dialog
    //

    settingDialog.InitWidgets();
}

void MainWindow::RegisterWidgetEventHandler()
{
    //
    // setting dialog
    //

    settingDialog.RegisterSettingChangedHandler(std::bind(&MainWindow::SettingChanged, this, std::placeholders::_1, std::placeholders::_2));

    //
    // wechat status table view
    //

    ui->viewWeChatStatus->viewport()->installEventFilter(this);
    QObject::connect(ui->viewWeChatStatus->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection& /*deselected*/) {
        bool empty = selected.isEmpty();
        ui->lineCommand->setEnabled(!empty);
        if (!empty) {
            ui->lineCommand->setFocus();
        }
    });
    QObject::connect(ui->viewWeChatStatus, &QWidget::customContextMenuRequested, this, [this](QPoint pos) {
        wb_process::PID pid = wxStatusModel.selection();
        if (!pid) {
            return;
        }

        auto item = wxStatusModel.get(pid);
        if (!item) {
            return;
        }

        QString nickname = item->nickname;
        QString wxnumber = item->wxnumber;
        QString wxid     = item->wxid;

        if (item->status == WxBoxClientItemStatus::Independent) {
            clientItemContextMenu.connectAction("Inject", this, [this, pid]() {
                controller.InjectWxBotModule(pid);
            });
            clientItemContextMenu.show("Inject");
            clientItemContextMenu.hide("UnInject");
            clientItemContextMenu.hide("Show Feature Info");
        }
        else {
            clientItemContextMenu.connectAction("UnInject", this, [this, pid]() {
                controller.UnInjectWxBotModule(pid);
            });
            clientItemContextMenu.show("UnInject");
            clientItemContextMenu.hide("Inject");

            if (item->status == WxBoxClientItemStatus::Normal && controller.clientInjectArgs.find(pid) != controller.clientInjectArgs.end()) {
                clientItemContextMenu.connectAction("Show Feature Info", this, [this, pid]() {
                    controller.DisplayClientInjectArgs(pid);
                });
                clientItemContextMenu.show("Show Feature Info");
            }
        }

        if (item->logined) {
            clientItemContextMenu.enable("Copy NickName");
            clientItemContextMenu.enable("Copy WxNumber");
            clientItemContextMenu.enable("Copy WXID");
            clientItemContextMenu.enable("Refresh Profile");
            clientItemContextMenu.enable("All Contact");
            clientItemContextMenu.enable("Logout");

            clientItemContextMenu.connectAction("Copy NickName", this, [this, nickname]() {
                QApplication::clipboard()->setText(nickname);
            });
            clientItemContextMenu.connectAction("Copy WxNumber", this, [this, wxnumber]() {
                QApplication::clipboard()->setText(wxnumber);
            });
            clientItemContextMenu.connectAction("Copy WXID", this, [this, wxid]() {
                QApplication::clipboard()->setText(wxid);
            });
            clientItemContextMenu.connectAction("Refresh Profile", this, [this, pid]() {
                controller.RequestProfile(pid);
            });
            clientItemContextMenu.connectAction("All Contact", this, [this, pid]() {
                controller.RequstAllContact(pid);
            });
            clientItemContextMenu.connectAction("Logout", this, [this, pid]() {
                controller.RequstLogoutWeChat(pid);
            });
        }
        else {
            clientItemContextMenu.disable("Copy NickName");
            clientItemContextMenu.disable("Copy WxNumber");
            clientItemContextMenu.disable("Copy WXID");
            clientItemContextMenu.disable("Refresh Profile");
            clientItemContextMenu.disable("All Contact");
            clientItemContextMenu.disable("Logout");
        }

        clientItemContextMenu.connectAction("Raise To Foreground", this, [this, pid]() {
            controller.RaiseClientWindowToForeground(pid);
        });

        clientItemContextMenu.popup(this->ui->viewWeChatStatus->viewport()->mapToGlobal(pos));
    });

    //
    // command line
    //

    ui->lineCommand->RegisterExecuteHandler([this](const QString& statement) {
        // clear line result screen
        if (!statement.compare("wxbox.clear")) {
            ui->viewCommandExecuteLogger->clear();
            ui->lineCommand->clear();
            return;
        }

        wb_process::PID pid = wxStatusModel.selection();
        if (!pid) {
            AppendExecuteCommandResult(QString("[<font color=\"blue\">WxBox</font>] : %1").arg(Translate("No client has been selected")));
            ui->lineCommand->clear();
            return;
        }

        auto client = wxStatusModel.get(pid);
        if (!client || client->status != WxBoxClientItemStatus::Normal) {
            AppendExecuteCommandResult(QString("[<font color=\"blue\">WxBox</font>] : [<font color=\"red\">%1</font>] %2").arg(pid).arg(Translate("Client not connected")));
            ui->lineCommand->clear();
            return;
        }

        this->controller.RequestExecutePluginScript(pid, ">>" + statement.toStdString());
        ui->lineCommand->clear();
    });

    //
    // only for test
    //

    QObject::connect(ui->btn_about, &QPushButton::clicked, &aboutDialog, &AboutWxBoxDialog::showApplicationModal);
    QObject::connect(ui->btnUpdateFeatureRepository, &QPushButton::clicked, this, &MainWindow::UpdateWeChatFeatures);
    QObject::connect(ui->btnStartWeChat, &QPushButton::clicked, &this->controller, &WxBoxController::StartWeChatInstance);

    QObject::connect(ui->btn_test1, &QPushButton::clicked, this, [this]() {
        int* p = nullptr;
        *p     = 2;

        /*xstyle::warning(nullptr, "wraning", "change to english and DefaultTheme", XStyleMessageBoxButtonType::Ok);
        xstyle::warning(nullptr, "wraning", "change to english and DefaultTheme", XStyleMessageBoxButtonType::Ok);
        xstyle::warning(nullptr, "wraning", "change to english and DefaultTheme", XStyleMessageBoxButtonType::Ok);
        xstyle::warning(nullptr, "wraning", "change to english and DefaultTheme", XStyleMessageBoxButtonType::Ok);*/
        /*    xstyle_manager.ChangeLanguage("zh_cn");
        xstyle_manager.ChangeTheme("");*/
    });
    QObject::connect(ui->btn_test2, &QPushButton::clicked, this, [this]() {
        /* xstyle::message(this, "message", "it's a message", XStyleMessageBoxButtonType::NoButton);
        xstyle::error(nullptr, "error", "ready to crash", XStyleMessageBoxButtonType::Ok);
        char* e = nullptr;
        *e      = 0;*/
    });
    QObject::connect(ui->btn_test3, &QPushButton::clicked, this, [this]() {
        /*    xstyle::information(this, "information", "change to chinese and GreenTheme");
        xstyle_manager.ChangeLanguage("zh_cn");
        xstyle_manager.ChangeTheme("GreenTheme");*/
        /*       xstyle_manager.ChangeTheme("");
        xstyle_manager.ChangeLanguage("zh_cn");*/
        //controller.ChangeWeChatStatusMonitorInterval(2000);
        /*controller.StopWeChatStatusMonitor();
        wxStatusModel.clear();*/

        ModifySetting();
    });
    QObject::connect(ui->btn_test4, &QPushButton::clicked, this, [this]() {
        //UpdateWeChatFeatures();
        //xstyle_manager.ChangeLanguage("en");

        xstyle::information(this, "information", "change to chinese and GreenTheme");
        xstyle::information(nullptr, "information", "change to chinese and GreenTheme");

        /*  WXBOT_LOG_INFO("hello {}", "information");
        WXBOT_LOG_WARNING("hello {}", "warning");
        WXBOT_LOG_ERROR("hello {}", "error");
        WXBOX_LOG_INFO_AND_SHOW_MSG_BOX(this, "hi", "is a message {}", "information");
        WXBOX_LOG_WARNING_AND_SHOW_MSG_BOX(this, "hi", "is a message {}", "warning");
        WXBOX_LOG_ERROR_AND_SHOW_MSG_BOX(this, "hi", "is a message {}", " error");*/
    });
}

bool MainWindow::InitWxBox(QSplashScreen* splash)
{
    // press close button is minimize to tray not quit
    TurnCloseIsMinimizeToTray(config.close_is_minimize_to_tray());

    // wxbox window whether always top most
    TurnAlwaysTopMost(config.always_top_most());

    // loading icon
    SetWindowLoadingIconType(WindowLoadingIconType(config.loading_icon_type()));
    SetUseLoadingIconAnimationCache(config.loading_icon_animation_use_cache());

    // init app menu
    SPLASH_MESSAGE("Init Application Menu");
    InitAppMenu();

    // init app tray
    SPLASH_MESSAGE("Init Application Tray");
    InitAppTray();

    // init widget
    SPLASH_MESSAGE("Init Widget");
    InitWidget();

    // register widget event handelr
    SPLASH_MESSAGE("Register Widget Event");
    RegisterWidgetEventHandler();

    // load wechat environment info
    SPLASH_MESSAGE("Load WeChat Environment Info");
    controller.LoadWeChatEnvironmentInfo();

    // preload features
    SPLASH_MESSAGE("Preload WeChat API Features");
    controller.ReloadFeatures();

    // start wxbox server
    SPLASH_MESSAGE("Start WxBox RPC Server");
    controller.StartWxBoxServer();
    return true;
}

bool MainWindow::DeinitWxBox()
{
    // stop wechat status monitor
    controller.StopWeChatStatusMonitor();

    // stop wxbox server
    controller.StopWxBoxServer();

    // save command history
    config.save_plugin_command_history(ui->lineCommand->SaveHistory(config.plugin_command_max_history_persistence_line()));
    return true;
}