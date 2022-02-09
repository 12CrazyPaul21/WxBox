#include "mainwindow.h"
#include "ui_mainwindow.h"

#define WXBOX_MAIN_WINDOW_NAME "MainWindow"
#define WXBOX_MAIN_WINDOW_TITLE "WxBox"
#define WXBOX_ICON_URL ":/icon/app.ico"

MainWindow::MainWindow(QWidget* parent)
  : XSTYLE_WINDOW_CLASS(WXBOX_MAIN_WINDOW_NAME, parent, false)
  , ui(new Ui::MainWindowBody)
  , config(AppConfig::singleton())
  , aboutDialog(this)
  , appMenu("AppMenu", this)
  , appTray(this)
  , controller(this)
  , downloader(this)
{
    // setup xstyle ui
    qApp->setStyle(QStyleFactory::create("Fusion"));
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

void MainWindow::RegisterEvent()
{
    QObject::connect(ui->btn_about, &QPushButton::clicked, &aboutDialog, &AboutWxBoxDialog::showApplicationModal);

    QObject::connect(ui->btn_test1, &QPushButton::clicked, this, [this]() {
        xstyle::warning(this, "wraning", "change to english and DefaultTheme", XStyleMessageBoxButtonType::Ok);
        xstyle_manager.ChangeLanguage("en");
        xstyle_manager.ChangeTheme("");
    });
    QObject::connect(ui->btn_test2, &QPushButton::clicked, this, [this]() {
        xstyle::message(this, "message", "it's a message", XStyleMessageBoxButtonType::NoButton);
        xstyle::error(nullptr, "error", "ready to crash", XStyleMessageBoxButtonType::Ok);
        char* e = nullptr;
        *e      = 0;
    });
    QObject::connect(ui->btn_test3, &QPushButton::clicked, this, [this]() {
        /*    xstyle::information(this, "information", "change to chinese and GreenTheme");
        xstyle_manager.ChangeLanguage("zh_cn");
        xstyle_manager.ChangeTheme("GreenTheme");*/
        downloader.cancel();
    });
    QObject::connect(ui->btn_test4, &QPushButton::clicked, this, [this]() {
        QUrl featureSetUrl("https://gitee.com/phantom27/wxbox-public-storage/attach_files/917653/download/lua-5.4.3.zip");
        downloader.download(
            featureSetUrl,
            [this](const QUrl& url, qint64 progress, qint64 total) {
                ui->progressBar->setValue(progress);
                ui->progressBar->setMaximum(total);
            },
            [this](const QUrl& url, const QByteArray& bytes) { xstyle::information(this, "title", bytes); },
            [this](const QUrl& url, const QNetworkReply::NetworkError& error, const QString& errorString) { xstyle::error(this, "", errorString); });
    });

    ui->closeIsMinimizeTray->setChecked(config.close_is_minimize_to_tray());
    QObject::connect(ui->closeIsMinimizeTray, &QCheckBox::stateChanged, this, [this](int state) {
        TurnCloseIsMinimizeToTray(state == Qt::Checked);
    });
}

void MainWindow::InitAppMenu()
{
    appMenu.pushAction("setting");
    appMenu.pushSeparator();
    appMenu.pushAction("visit repository", this, std::bind(&QDesktopServices::openUrl, QUrl(WXBOX_REPOSITORY_URL)));
    appMenu.pushAction("about wxbox", &aboutDialog, std::bind(&AboutWxBoxDialog::showApplicationModal, &aboutDialog));
    appMenu.pushSeparator();
    appMenu.pushAction("exit wxbox", this, std::bind(&MainWindow::quit, this));
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

bool MainWindow::InitWxBox()
{
    // press close button is minimize to tray not quit
    TurnCloseIsMinimizeToTray(config.close_is_minimize_to_tray());

    // init app menu
    InitAppMenu();

    // init app tray
    InitAppTray();

    // register event
    RegisterEvent();

    // start wxbox server
    controller.StartWxBoxServer();
    return true;
}

bool MainWindow::DeinitWxBox()
{
    // stop wxbox server
    controller.StopWxBoxServer();
    return true;
}