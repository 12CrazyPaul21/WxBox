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
  , server(nullptr)
  , statusMonitorTimerId(-1)
  , statusMonitorInterval(0)
  , lastUpdateStatusTimestamp(0)
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

    server = wxbox::WxBoxServer::NewWxBoxServer(config.wxbox_server_uri());

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

void WxBoxController::StartWeChatStatusMonitor()
{
    UpdateWeChatStatus();
    statusMonitorInterval = std::max<int>(config.wechat_status_monitor_interval(), 100);
    statusMonitorTimerId  = view->startTimer(statusMonitorInterval);
}

void WxBoxController::StopWeChatStatusMonitor()
{
    if (statusMonitorTimerId != -1) {
        view->killTimer(statusMonitorTimerId);
        statusMonitorTimerId = -1;
    }
}

void WxBoxController::ChangeWeChatStatusMonitorInterval(int interval)
{
    config.change_wechat_status_monitor_interval(interval);
    StopWeChatStatusMonitor();
    StartWeChatStatusMonitor();
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

bool WxBoxController::StartWeChatInstance()
{
    view->BeginMission();

    // verify wechat environment info
    if (!RequireValidWeChatEnvironmentInfo()) {
        xstyle::warning(view, "", WBC_TRANSMESSAGE(WBCErrorCode::INVALID_WECHAT_ENV_INFO));
        view->CloseMission();
        return false;
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

        // get process handle
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
        strcpy_s(wxbotEntryParameter.plugins_root, sizeof(wxbotEntryParameter.plugins_root), config.plugins_root().data());
        strcpy_s(wxbotEntryParameter.wxbox_server_uri, sizeof(wxbotEntryParameter.wxbox_server_uri), config.wxbox_server_uri().data());
        wxbotEntryParameter.wxbot_reconnect_interval = config.wxbox_client_reconnect_interval();
        wxbotEntryParameter.plugin_long_task_timeout = config.plugin_long_task_timeout();
        wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis);

        // log wx core module info
        spdlog::info("WeChat Core Module baseaddr : 0x{:08X}, size : 0x{:08X}", (ucpulong_t)openResult.pModuleBaseAddr, openResult.uModuleSize);

        // log wx hook point
        WXBOX_LOG_WECHAT_APIS(wxbotEntryParameter.wechat_apis);

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
    return errorCode == WBCErrorCode::WBC_NO_ERROR;
}

bool WxBoxController::InjectWxBotModule(wb_process::PID pid)
{
    if (wb_crack::IsWxBotInjected(pid)) {
        xstyle::information(view, "", WBC_TRANSMESSAGE(WBCErrorCode::WECHAT_PROCESS_IS_ALREADY_INJECT));
        return false;
    }

    view->BeginMission();

    WBCErrorCode errorCode = WBCErrorCode::WBC_NO_ERROR;

    wxbox::internal::TaskInThreadPool::StartTask([this, pid, &errorCode]() {
        // get wechat process environment info
        wb_wx::WeChatProcessEnvironmentInfo wxProcessEnvInfo;
        if (!wb_wx::ResolveWxEnvInfo(pid, wxProcessEnvInfo)) {
            errorCode = WBCErrorCode::RESOLVE_WECHAT_PROCESS_ENV_INFO_FAILED;
            return;
        }

        // get process handle
        wb_process::AutoProcessHandle aphandle = wb_process::OpenProcessAutoHandle(pid);
        if (!aphandle.valid()) {
            errorCode = WBCErrorCode::GET_WECHAT_PROCESS_HANDLE_FAILED;
            return;
        }

        // collect hook point
        wb_feature::LocateTarget               locateTarget = {aphandle.hProcess, wxProcessEnvInfo.pCoreModuleBaseAddr, wxProcessEnvInfo.uCoreModuleSize};
        wb_feature::WxAPIHookPointVACollection vaCollection;
        wxApiFeatures.Collect(locateTarget, wxProcessEnvInfo.wxEnvInfo.version, vaCollection);
        aphandle.close();

        //
        // inject wxbot
        //

        // wxbot entry parameter
        wb_crack::WxBotEntryParameter wxbotEntryParameter;
        std::memset(&wxbotEntryParameter, 0, sizeof(wxbotEntryParameter));
        wxbotEntryParameter.wxbox_pid = wb_process::GetCurrentProcessId();
        strcpy_s(wxbotEntryParameter.wxbox_root, sizeof(wxbotEntryParameter.wxbox_root), wb_file::GetProcessRootPath().data());
        strcpy_s(wxbotEntryParameter.wxbot_root, sizeof(wxbotEntryParameter.wxbot_root), config.wxbot_root_path().data());
        strcpy_s(wxbotEntryParameter.plugins_root, sizeof(wxbotEntryParameter.plugins_root), config.plugins_root().data());
        strcpy_s(wxbotEntryParameter.wxbox_server_uri, sizeof(wxbotEntryParameter.wxbox_server_uri), config.wxbox_server_uri().data());
        wxbotEntryParameter.wxbot_reconnect_interval = config.wxbox_client_reconnect_interval();
        wxbotEntryParameter.plugin_long_task_timeout = config.plugin_long_task_timeout();
        wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis);

        // log wx core module info
        spdlog::info("Inject to WeChat Process(PID : {})", pid);
        spdlog::info("WeChat Version : {}", wxProcessEnvInfo.wxEnvInfo.version);
        spdlog::info("WeChat Install Path : {}", wxProcessEnvInfo.wxEnvInfo.installPath);
        spdlog::info("WeChat Core Module baseaddr : 0x{:08X}, size : 0x{:08X}", (ucpulong_t)wxProcessEnvInfo.pCoreModuleBaseAddr, wxProcessEnvInfo.uCoreModuleSize);

        // log wx hook point
        WXBOX_LOG_WECHAT_APIS(wxbotEntryParameter.wechat_apis);

        // verify hook point
        if (!wb_crack::VerifyWxApis(wxbotEntryParameter.wechat_apis)) {
            errorCode = WBCErrorCode::WECHAT_API_HOOK_POINT_INVALID;
            return;
        }

        // inject
        if (!wb_crack::InjectWxBot(pid, wxbotEntryParameter)) {
            errorCode = WBCErrorCode::INJECT_WXBOT_MODULE_FAILED;
            return;
        }
    })
        .wait();

    if (WBC_FAILED(errorCode)) {
        WXBOX_LOG_ERROR_AND_SHOW_MSG_BOX(view, "", WBC_MESSAGE(errorCode));
    }

    view->CloseMission();
    return errorCode == WBCErrorCode::WBC_NO_ERROR;
}

bool WxBoxController::UnInjectWxBotModule(wb_process::PID pid)
{
    if (!wb_crack::IsWxBotInjected(pid)) {
        xstyle::information(view, "", WBC_TRANSMESSAGE(WBCErrorCode::WECHAT_PROCESS_IS_NOT_INJECTED));
        return false;
    }

    if (!server->IsClientAlive(pid)) {
        xstyle::information(view, "", WBC_TRANSMESSAGE(WBCErrorCode::WECHAT_PROCESS_WXBOT_MODULE_NOT_CONNECTED));
        return false;
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = pid;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::UNINJECT_WXBOT_REQUEST);
    PushMessageAsync(std::move(msg));
    return true;
}

//
// WeChat Status
//

void WxBoxController::UpdateClientStatus(wb_process::PID pid) const noexcept
{
    auto client = view->wxStatusModel.get(pid);
    if (!client) {
        return;
    }

    client->status = GetClientStatus(pid);
    client->update();
}

void WxBoxController::UpdateWeChatStatus()
{
    time_t timestamp = wb_process::GetCurrentTimestamp();
    if (timestamp - lastUpdateStatusTimestamp < statusMonitorInterval) {
        return;
    }
    lastUpdateStatusTimestamp = timestamp;

    auto                      wxProcessList = wb_wx::GetWeChatProcessIdList();
    auto&                     model         = view->wxStatusModel;
    std::set<wb_process::PID> invalidClients;

    for (const auto& [pid, itemPtr] : model.all()) {
        if (!itemPtr) {
            continue;
        }

        auto it = std::find_if(wxProcessList.begin(), wxProcessList.end(), [pid](const wb_process::PID wxPid) {
            return wxPid == pid;
        });

        // invalid client
        if (it == wxProcessList.end()) {
            invalidClients.insert(pid);
            continue;
        }

        //
        // check update
        //

        bool dirty = false;
        wxProcessList.erase(it);

        WxBoxClientItemStatus newStatus = GetClientStatus(pid);
        if (itemPtr->status != newStatus) {
            itemPtr->status = newStatus;
            dirty           = true;
        }

        // execute update
        if (dirty) {
            itemPtr->update();
        }
    }

    // delete invalid client items
    model.removes(invalidClients);

    // add new wechat process items
    for (auto pid : wxProcessList) {
        auto item    = WxBoxClientStatusItem::New();
        item->pid    = pid;
        item->status = GetClientStatus(pid);
        model.add(item);
    }
}

//
// WxBoxServer status changed and event slots
//

void WxBoxController::WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus)
{
    WXBOX_UNREF(oldStatus);

    switch (newStatus) {
        case WxBoxServerStatus::Started:
            StartWeChatStatusMonitor();
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
            UpdateClientStatus(message.pid);

            // only for test
            last_client_pid = message.pid;
            total_clients++;
            current_clients++;
            view->SetWindowTitle(QString("total client count : <%1>, active client count : <%2>").arg(total_clients).arg(current_clients));
            break;

        case wxbox::WxBoxMessageType::WxBoxClientDone:
            UpdateClientStatus(message.pid);

            // only for test
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