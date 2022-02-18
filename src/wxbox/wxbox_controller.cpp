#include <wxbox_controller.h>
#include <wxbox_controller_error.h>
#include <mainwindow.h>

#define _wctr(MESSAGE) Translate(MESSAGE)

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

    wxbox::WxBoxServer* server = wxbox::WxBoxServer::NewWxBoxServer(config.wxbox_server_uri());

    // connect slots
    QObject::connect(&worker, SIGNAL(finished()), server, SLOT(deleteLater()));
    QObject::connect(&worker, SIGNAL(shutdown()), server, SLOT(shutdown()), Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerStatusChange, this, &WxBoxController::WxBoxServerStatusChange, Qt::QueuedConnection);
    QObject::connect(this, &WxBoxController::PushMessageAsync, server, &wxbox::WxBoxServer::PushMessageAsync, Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerEvent, this, &WxBoxController::WxBoxServerEvent, Qt::QueuedConnection);

    // start WxBoxServer
    worker.startServer(server);
    spdlog::info("WxBox Server is running");

    // prevent the window from being closed before the service ends
    view->IgnoreForClose();
}

void WxBoxController::StopWxBoxServer()
{
    if (worker.isRunning()) {
        worker.stopServer();
        spdlog::info("WxBox Server is already stoped");
    }
}

void WxBoxController::LoadWeChatEnvironmentInfo()
{
    if (wb_wx::ResolveWxEnvInfo(config.wechat_installation_dir(), config.wechat_module_dir(), wxEnvInfo)) {
        return;
    }

    if (wb_wx::ResolveWxEnvInfo(wxEnvInfo)) {
        config.change_wechat_installation_dir(wxEnvInfo.installPath);
        config.change_wechat_module_dir(wxEnvInfo.moduleFolderAbsPath);
    }
}

bool WxBoxController::RequireValidWeChatEnvironmentInfo()
{
    if (wb_wx::IsWxInstallationPathValid(wxEnvInfo.installPath, wxEnvInfo.moduleFolderAbsPath)) {
        return true;
    }

    // Supplement the dialog box to specify a valid installation path
    // ...

    return false;
}

void WxBoxController::ReloadFeatures()
{
    wb_feature::PreLoadFeatures(config.features_path(), wxApiFeatures);
    spdlog::info("Load WxBox api features");
}

void WxBoxController::StartWeChatInstance()
{
    view->BeginMission();

    // verify wechat environment info
    if (!RequireValidWeChatEnvironmentInfo()) {
        xstyle::warning(view, "", _wctr(WBC_MESSAGE(WBCErrorCode::INVALID_WECHAT_ENV_INFO)));
        view->CloseMission();
        return;
    }

    WBCErrorCode errorCode     = WBCErrorCode::WBC_NO_ERROR;
    bool         isInjectWxBot = xstyle::information(view, "", _wctr("Do you want to inject wxbot at the same time?"), XStyleMessageBoxButtonType::YesNo) == XStyleMessageBoxButton::Yes;

    wxbox::internal::TaskInThreadPool::StartTask([this, isInjectWxBot, &errorCode]() {
        // open and wechat with multi boxing
        wb_crack::OpenWxWithMultiBoxingResult openResult = {0};
        if (!wxbox::crack::OpenWxWithMultiBoxing(wxEnvInfo, wxApiFeatures, &openResult, isInjectWxBot)) {
            errorCode = WBCErrorCode::OPEN_WECHAT_FAILED;
            return;
        }
        spdlog::info("new wechat process pid : {}, version : {}", openResult.pid, wxEnvInfo.version);

        if (!isInjectWxBot) {
            return;
        }

        // get process info
        wb_process::AutoProcessHandle aphandle = wb_process::OpenProcessAutoHandle(openResult.pid);
        if (!aphandle.valid()) {
            errorCode = WBCErrorCode::GET_WECHAT_PROCESS_HANDLE_FAILED;
            return;
        }

        // collect hook point
        wb_feature::LocateTarget               locateTarget = {aphandle.hProcess, openResult.pModuleBaseAddr, openResult.uModuleSize};
        wb_feature::WxAPIHookPointVACollection vaCollection;
        wxApiFeatures.Collect(locateTarget, wxEnvInfo.version, vaCollection);
        aphandle.close();

        // deattach
        wb_crack::DeAttachWxProcess(openResult.pid);

        //
        // inject wxbot
        //

        // wxbot entry parameter
        wb_crack::WxBotEntryParameter wxbotEntryParameter;
        std::memset(&wxbotEntryParameter, 0, sizeof(wxbotEntryParameter));
        wxbotEntryParameter.wxbox_pid = wb_process::GetCurrentProcessId();
        strcpy_s(wxbotEntryParameter.wxbox_root, sizeof(wxbotEntryParameter.wxbox_root), wb_file::GetProcessRootPath().data());
        strcpy_s(wxbotEntryParameter.wxbot_root, sizeof(wxbotEntryParameter.wxbot_root), config.wxbot_root_path().data());
        strcpy_s(wxbotEntryParameter.wxbox_server_uri, sizeof(wxbotEntryParameter.wxbox_server_uri), config.wxbox_server_uri().data());
        wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis);

        // log wx core module info
        spdlog::info("WeChat Core Module baseaddr : 0x{:08X}, size : 0x{:08X}", (ucpulong_t)openResult.pModuleBaseAddr, openResult.uModuleSize);

        // log wx hook point
        spdlog::info("CheckAppSingleton va : 0x{:08X}", wxbotEntryParameter.wechat_apis.CheckAppSingleton);
        spdlog::info("FetchGlobalContactContextAddress va : 0x{:08X}", wxbotEntryParameter.wechat_apis.FetchGlobalContactContextAddress);
        spdlog::info("InitWeChatContactItem va : 0x{:08X}", wxbotEntryParameter.wechat_apis.InitWeChatContactItem);
        spdlog::info("DeinitWeChatContactItem va : 0x{:08X}", wxbotEntryParameter.wechat_apis.DeinitWeChatContactItem);
        spdlog::info("FindAndDeepCopyWeChatContactItemWithWXIDWrapper va : 0x{:08X}", wxbotEntryParameter.wechat_apis.FindAndDeepCopyWeChatContactItemWithWXIDWrapper);
        spdlog::info("FetchGlobalProfileContext va : 0x{:08X}", wxbotEntryParameter.wechat_apis.FetchGlobalProfileContext);
        spdlog::info("HandleRawMessages va : 0x{:08X}", wxbotEntryParameter.wechat_apis.HandleRawMessages);
        spdlog::info("HandleReceivedMessages va : 0x{:08X}", wxbotEntryParameter.wechat_apis.HandleReceivedMessages);
        spdlog::info("WXSendTextMessage va : 0x{:08X}", wxbotEntryParameter.wechat_apis.WXSendTextMessage);
        spdlog::info("FetchGlobalSendMessageContext va : 0x{:08X}", wxbotEntryParameter.wechat_apis.FetchGlobalSendMessageContext);
        spdlog::info("WXSendFileMessage va : 0x{:08X}", wxbotEntryParameter.wechat_apis.WXSendFileMessage);

        // verify hook point
        if (!wb_crack::VerifyWxApis(wxbotEntryParameter.wechat_apis)) {
            errorCode = WBCErrorCode::WECHAT_API_HOOK_POINT_INVALID;
            return;
        }

        // inject
        if (!wb_crack::InjectWxBot(openResult.pid, wxbotEntryParameter)) {
            errorCode = WBCErrorCode::INJECT_WXBOT_MODULE_FAILED;
            return;
        }
    })
        .wait();

    if (WBC_FAILED(errorCode)) {
        WXBOX_LOG_ERROR_AND_SHOW_MSG_BOX(view, "", WBC_MESSAGE(errorCode));
    }

    view->CloseMission();
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
                WXBOX_LOG_ERROR_AND_SHOW_MSG_BOX(view, "", WBC_MESSAGE(WBCErrorCode::OPEN_WXBOX_SERVER_FAILED));
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