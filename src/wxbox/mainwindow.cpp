#include "mainwindow.h"
#include "ui_mainwindow.h"

static wb_process::PID last_client_pid = 0;
static uint64_t        total_clients   = 0;
static uint64_t        current_clients = 0;

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , aboutDialog(this)
  , ui(new Ui::MainWindow)
  , config(AppConfig::singleton())
  , init(false)
  , wantToClose(false)
  , readyForCloseCounter(0)
{
    changeLanguage(config.language());
    ui->setupUi(this);

    // set thread name
    wb_process::SetThreadName(wb_process::GetCurrentThreadHandle(), "WxBoxMain");

    // change style
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // ---------------------------------------
    //           trigger to test
    // ---------------------------------------

    auto button = new QPushButton("test", ui->centralWidget);
    connect(button, SIGNAL(clicked()), this, SLOT(triggerTest()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//
// Events
//

void MainWindow::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            QMainWindow::changeEvent(event);
    }
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!init) {
        init = true;
        QTimer::singleShot(10, this, SLOT(InitWxBox()));
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (init) {
        emit DeinitWxBox();
        init = false;
    }

    wantToClose = true;
    readyForCloseCounter <= 0 ? event->accept() : event->ignore();
}

//
// Slots
//

void MainWindow::InitWxBox()
{
    startWxBoxServer();
}

void MainWindow::DeinitWxBox()
{
    stopWxBoxServer();
}

void MainWindow::OpenAboutDialog()
{
    aboutDialog.show();
}

void MainWindow::WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus)
{
    WXBOX_UNREF(oldStatus);

    switch (newStatus) {
        case WxBoxServerStatus::Started:
            break;
        case WxBoxServerStatus::StartServiceFailed:
            readyForClose(false);
            QTimer::singleShot(100, this, [&]() {
                QMessageBox::information(this, "Warning!!!", "start WxBoxServer failed, close application now...");
                emit this->close();
            });
            break;
        case WxBoxServerStatus::Interrupted:
        case WxBoxServerStatus::Stopped:
            readyForClose();
            break;
    }
}

void MainWindow::WxBoxServerEvent(wxbox::WxBoxMessage message)
{
    switch (message.type) {
        case wxbox::WxBoxMessageType::WxBoxClientConnected:
            last_client_pid = message.pid;
            total_clients++;
            current_clients++;
            setWindowTitle(QString("total client count : <%1>, active client count : <%2>").arg(total_clients).arg(current_clients));
            break;
        case wxbox::WxBoxMessageType::WxBoxClientDone:
            last_client_pid = 0;
            current_clients--;
            setWindowTitle(QString("total client count : <%1>, active client count : <%2>").arg(total_clients).arg(current_clients));
            break;
        case wxbox::WxBoxMessageType::WxBotRequestOrResponse: {
            ProfileResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_profileresponse());
            break;
        }
    }
}

//
// Methods
//

bool MainWindow::checkSystemVersionSupported()
{
    bool isSupported = false;

#if WXBOX_IN_WINDOWS_OS
    static QString msg = tr("Only systems above Windows 7 are supported.");
    isSupported        = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows7;
#else
    static QString msg = tr("Only systems above Mac OS Sierra are supported.");
    isSupported        = QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSierra;
#endif

    if (!isSupported) {
        QMessageBox::information(this, tr(""), msg);
    }

    return isSupported;
}

std::vector<std::pair<std::string, std::string>> MainWindow::i18ns()
{
    std::vector<std::pair<std::string, std::string>> result;
    auto                                             i18nPath = config.i18n_path();

    if (!wb_file::IsDirectory(i18nPath)) {
        return result;
    }

    for (auto qm : wb_file::ListFilesInDirectoryWithExt(i18nPath, "qm")) {
        QTranslator trans;
        if (trans.load(wb_file::JoinPath(config.i18n_path(), qm).c_str())) {
            result.emplace_back(std::make_pair<std::string, std::string>(wb_file::ExtractFileNameAndExt(qm).first,
                                                                         trans.translate("MainWindow", "English").toUtf8().toStdString()));
        }
    }

    return result;
}

void MainWindow::changeLanguage(const std::string& language)
{
    wb_coredump::ChangeDumperLanguage(language);

    auto languagePath = wb_file::JoinPath(config.i18n_path(), language + ".qm");
    if (!wb_file::IsPathExists(languagePath)) {
        return;
    }

    if (translator.load(QString::fromLocal8Bit(languagePath.c_str()))) {
        qApp->installTranslator(&translator);
    }
}

void MainWindow::ignoreForClose()
{
    readyForCloseCounter.ref();
}

void MainWindow::readyForClose(const bool canCloseWhenZero)
{
    readyForCloseCounter.deref();
    if (wantToClose && readyForCloseCounter <= 0 && canCloseWhenZero) {
        emit this->close();
    }
}

void MainWindow::startWxBoxServer()
{
    if (worker.isRunning()) {
        return;
    }

    wxbox::WxBoxServer* server = wxbox::WxBoxServer::NewWxBoxServer();

    // connect slots
    QObject::connect(&worker, SIGNAL(finished()), server, SLOT(deleteLater()));
    QObject::connect(&worker, SIGNAL(shutdown()), server, SLOT(shutdown()), Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerStatusChange, this, &MainWindow::WxBoxServerStatusChange, Qt::QueuedConnection);
    QObject::connect(this, &MainWindow::PushMessageAsync, server, &wxbox::WxBoxServer::PushMessageAsync, Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerEvent, this, &MainWindow::WxBoxServerEvent, Qt::QueuedConnection);

    // start WxBoxServer
    worker.startServer(server);

    // prevent the window from being closed before the service ends
    ignoreForClose();

    spdlog::info("WxBox Server is running");
}

void MainWindow::stopWxBoxServer()
{
    if (worker.isRunning()) {
        worker.stopServer();
    }

    spdlog::info("WxBox Server is already stop");
}

//
// WxBoxServer Wrapper Request Methods
//

void MainWindow::RequestProfile(wb_process::PID clientPID)
{
    if (!clientPID) {
        return;
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::PROFILE_REQUEST);
    PushMessageAsync(std::move(msg));
}

//
// WxBoxServer Response Handler
//

void MainWindow::ProfileResponseHandler(wb_process::PID clientPID, wxbox::ProfileResponse* response)
{
    std::string wxid = response->wxid();
    last_client_pid  = clientPID;
    QMessageBox::information(this, "tips", QString("profile<%1> : %2").arg(last_client_pid).arg(QString::fromStdString(wxid.c_str())));
}

// ---------------------------------------
//           trigger to test
// ---------------------------------------

[[deprecated("just a test trigger")]] void MainWindow::triggerTest()
{
    SPDLOG_DEBUG("trigger test button");

    char* p = nullptr;
    *p      = 2;
    RequestProfile(last_client_pid);
}