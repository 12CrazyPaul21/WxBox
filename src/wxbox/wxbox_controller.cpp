#include <wxbox_controller.h>
#include <mainwindow.h>

#define ERROR_START_WXBOX_SERVER_FAILED "start WxBoxServer failed, close application now..."

static wb_process::PID last_client_pid = 0;
static uint64_t        total_clients   = 0;
static uint64_t        current_clients = 0;

WxBoxController::WxBoxController(MainWindow* view)
  : QObject(reinterpret_cast<QObject*>(view))
  , config(AppConfig::singleton())
  , view(view)
{
    // set thread name
    wb_process::SetThreadName(wb_process::GetCurrentThreadHandle(), "WxBoxMain");
}

WxBoxController::~WxBoxController()
{
}

//
// Methods
//

bool WxBoxController::CheckSystemVersionSupported()
{
#if WXBOX_IN_WINDOWS_OS
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows7;
#else
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSierra;
#endif
}

void WxBoxController::StartWxBoxServer()
{
    if (worker.isRunning()) {
        return;
    }

    wxbox::WxBoxServer* server = wxbox::WxBoxServer::NewWxBoxServer();

    // connect slots
    QObject::connect(&worker, SIGNAL(finished()), server, SLOT(deleteLater()));
    QObject::connect(&worker, SIGNAL(shutdown()), server, SLOT(shutdown()), Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerStatusChange, this, &WxBoxController::WxBoxServerStatusChange, Qt::QueuedConnection);
    QObject::connect(this, &WxBoxController::PushMessageAsync, server, &wxbox::WxBoxServer::PushMessageAsync, Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerEvent, this, &WxBoxController::WxBoxServerEvent, Qt::QueuedConnection);

    // start WxBoxServer
    worker.startServer(server);

    // prevent the window from being closed before the service ends
    view->IgnoreForClose();

    spdlog::info("WxBox Server is running");
}

void WxBoxController::StopWxBoxServer()
{
    if (worker.isRunning()) {
        worker.stopServer();
    }

    spdlog::info("WxBox Server is already stop");
}

void WxBoxController::ReloadFeatures()
{
    wb_feature::PreLoadFeatures(config.features_path(), wxApiFeatures);
    spdlog::info("Load WxBox api features");
}

//
// WxBoxServer status changed and event slots
//

void WxBoxController::WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus)
{
    WXBOX_UNREF(oldStatus);

    switch (newStatus) {
        case WxBoxServerStatus::Started:
            break;
        case WxBoxServerStatus::StartServiceFailed:
            view->ReadyForClose(false);
            QTimer::singleShot(100, this, [&]() {
                xstyle::error(view, "", ERROR_START_WXBOX_SERVER_FAILED);
                spdlog::error(ERROR_START_WXBOX_SERVER_FAILED);
                emit view->quit();
            });
            break;
        case WxBoxServerStatus::Interrupted:
        case WxBoxServerStatus::Stopped:
            view->ReadyForClose();
            break;
    }
}

void WxBoxController::WxBoxServerEvent(wxbox::WxBoxMessage message)
{
    switch (message.type) {
        case wxbox::WxBoxMessageType::WxBoxClientConnected:
            last_client_pid = message.pid;
            total_clients++;
            current_clients++;
            view->SetWindowTitle(QString("total client count : <%1>, active client count : <%2>").arg(total_clients).arg(current_clients));
            break;
        case wxbox::WxBoxMessageType::WxBoxClientDone:
            last_client_pid = 0;
            current_clients--;
            view->SetWindowTitle(QString("total client count : <%1>, active client count : <%2>").arg(total_clients).arg(current_clients));
            break;
        case wxbox::WxBoxMessageType::WxBotRequestOrResponse: {
            ProfileResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_profileresponse());
            break;
        }
    }
}

//
// WxBoxServer Wrapper Request Methods
//

void WxBoxController::RequestProfile(wb_process::PID clientPID)
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

void WxBoxController::ProfileResponseHandler(wb_process::PID clientPID, wxbox::ProfileResponse* response)
{
    std::string wxid = response->wxid();
    last_client_pid  = clientPID;
    QMessageBox::information(view, "tips", QString("profile<%1> : %2").arg(last_client_pid).arg(QString::fromStdString(wxid.c_str())));
}