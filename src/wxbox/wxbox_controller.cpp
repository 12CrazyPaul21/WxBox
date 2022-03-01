#include <wxbox_controller.h>
#include <wxbox_controller_error.h>
#include <mainwindow.h>

#define _wctr(MESSAGE) Translate(MESSAGE)

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
        server = nullptr;
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

void WxBoxController::ChangeWeChatInstallationPath(const std::string& path)
{
    config.change_wechat_installation_dir(path);
    LoadWeChatEnvironmentInfo();
}

void WxBoxController::ChangeWeChatCoreModulePath(const std::string& path)
{
    config.change_wechat_module_dir(path);
    LoadWeChatEnvironmentInfo();
}

void WxBoxController::LoadWeChatEnvironmentInfo()
{
    wxEnvInfo.installPath.clear();
    wxEnvInfo.moduleFolderAbsPath.clear();

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
    xstyle::warning(view, "", WBC_TRANSMESSAGE(WBCErrorCode::INVALID_WECHAT_ENV_INFO));
    view->RequestWeChatInstallationPathSetting();

    // recheck
    return wb_wx::IsWxInstallationPathValid(wxEnvInfo.installPath, wxEnvInfo.moduleFolderAbsPath);
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
        view->CloseMission();
        return false;
    }

    WBCErrorCode errorCode     = WBCErrorCode::WBC_NO_ERROR;
    auto         quesStatus    = xstyle::information(view, "", _wctr("Do you want to inject wxbot at the same time?"), XStyleMessageBoxButtonType::YesNo);
    bool         isInjectWxBot = (quesStatus == XStyleMessageBoxButton::Yes);

    if (quesStatus == XStyleMessageBoxButton::Close) {
        view->CloseMission();
        return false;
    }

    wxbox::internal::TaskInThreadPool::StartTask([this, isInjectWxBot, &errorCode]() {
        // open and wechat with multi boxing
        wb_crack::OpenWxWithMultiBoxingResult openResult = {0};
        if (!wxbox::crack::OpenWxWithMultiBoxing(wxEnvInfo, wxApiFeatures, &openResult, isInjectWxBot)) {
            errorCode = WBCErrorCode::OPEN_WECHAT_FAILED;
            return;
        }
        spdlog::info("New WeChat process pid : {}, version : {}", openResult.pid, wxEnvInfo.version);

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
        strcpy_s(wxbotEntryParameter.wechat_version, sizeof(wxbotEntryParameter.wechat_version), wxEnvInfo.version.c_str());
        strcpy_s(wxbotEntryParameter.wechat_install_path, sizeof(wxbotEntryParameter.wechat_install_path), wxEnvInfo.installPath.c_str());
        strcpy_s(wxbotEntryParameter.wechat_coremodule_abspath, sizeof(wxbotEntryParameter.wechat_coremodule_abspath), wxEnvInfo.coreModuleAbsPath.c_str());
        wxbotEntryParameter.avoidRevokeMessage        = config.wechat_avoid_revoke_message();
        wxbotEntryParameter.enableRawMessageHook      = config.wechat_enable_raw_message_hook();
        wxbotEntryParameter.enableSendTextMessageHook = config.wechat_enable_send_text_message_hook();
        wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis);
        std::memset(&wxbotEntryParameter.wechat_datastructure_supplement, 0, sizeof(wxbotEntryParameter.wechat_datastructure_supplement));
        wxApiFeatures.ObtainDataStructureSupplement(wxEnvInfo.version, wxbotEntryParameter.wechat_datastructure_supplement);

        // log wx core module info
        spdlog::info("WeChat Core Module baseaddr : 0x{:08X}, size : 0x{:08X}", (ucpulong_t)openResult.pModuleBaseAddr, openResult.uModuleSize);

        // log wx hook point
        WXBOX_LOG_WECHAT_APIS(wxbotEntryParameter.wechat_apis);

        // log wx datastructure supplement
        WXBOX_LOG_WECHAT_DATASTRUCTURE_SUPPLEMENT(wxbotEntryParameter.wechat_datastructure_supplement);

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
        strcpy_s(wxbotEntryParameter.wechat_version, sizeof(wxbotEntryParameter.wechat_version), wxProcessEnvInfo.wxEnvInfo.version.c_str());
        strcpy_s(wxbotEntryParameter.wechat_install_path, sizeof(wxbotEntryParameter.wechat_install_path), wxProcessEnvInfo.wxEnvInfo.installPath.c_str());
        strcpy_s(wxbotEntryParameter.wechat_coremodule_abspath, sizeof(wxbotEntryParameter.wechat_coremodule_abspath), wxProcessEnvInfo.wxEnvInfo.coreModuleAbsPath.c_str());
        wxbotEntryParameter.avoidRevokeMessage        = config.wechat_avoid_revoke_message();
        wxbotEntryParameter.enableRawMessageHook      = config.wechat_enable_raw_message_hook();
        wxbotEntryParameter.enableSendTextMessageHook = config.wechat_enable_send_text_message_hook();
        wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis);
        std::memset(&wxbotEntryParameter.wechat_datastructure_supplement, 0, sizeof(wxbotEntryParameter.wechat_datastructure_supplement));
        wxApiFeatures.ObtainDataStructureSupplement(wxProcessEnvInfo.wxEnvInfo.version, wxbotEntryParameter.wechat_datastructure_supplement);

        // log wx core module info
        spdlog::info("Inject to WeChat Process(PID : {})", pid);
        spdlog::info("WeChat Version : {}", wxProcessEnvInfo.wxEnvInfo.version);
        spdlog::info("WeChat Install Path : {}", wxProcessEnvInfo.wxEnvInfo.installPath);
        spdlog::info("WeChat Core Module baseaddr : 0x{:08X}, size : 0x{:08X}", (ucpulong_t)wxProcessEnvInfo.pCoreModuleBaseAddr, wxProcessEnvInfo.uCoreModuleSize);

        // log wx hook point
        WXBOX_LOG_WECHAT_APIS(wxbotEntryParameter.wechat_apis);

        // log wx datastructure supplement
        WXBOX_LOG_WECHAT_DATASTRUCTURE_SUPPLEMENT(wxbotEntryParameter.wechat_datastructure_supplement);

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

    if (!IsClientAlive(pid)) {
        xstyle::information(view, "", WBC_TRANSMESSAGE(WBCErrorCode::WECHAT_PROCESS_WXBOT_MODULE_NOT_CONNECTED));
        return false;
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = pid;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::UNINJECT_WXBOT_REQUEST);
    PushMessageAsync(std::move(msg));
    return true;
}

void WxBoxController::RaiseClientWindowToForeground(wb_process::PID pid)
{
    wb_wx::RaiseWeChatWindowToForeground(pid);
}

void WxBoxController::DisplayClientInjectArgs(wb_process::PID pid)
{
    if (clientInjectArgs.find(pid) == clientInjectArgs.end()) {
        return;
    }

    wb_crack::WxBotEntryParameter& injectArgs = clientInjectArgs.at(pid);

    QString     title = QString("WeChat %1").arg(xstyle_manager.Translate("xstyle_meta", "Feature"));
    QString     reportText;
    QTextStream reportTextStream(&reportText);

    reportTextStream << "WeChat Process ID : " << pid << "(0x" << Qt::hex << Qt::uppercasedigits << pid << ")" << XSTYLE_REPORT_ENDL;
    reportTextStream << "WeChat Version : " << injectArgs.wechat_version << XSTYLE_REPORT_ENDL;
    reportTextStream << "WeChat Install Path : " << injectArgs.wechat_install_path << XSTYLE_REPORT_ENDL;
    reportTextStream << "WeChat CoreModule : " << injectArgs.wechat_coremodule_abspath << XSTYLE_REPORT_ENDL
                     << XSTYLE_REPORT_ENDL;

    reportTextStream.setIntegerBase(16);
    reportTextStream.setNumberFlags(QTextStream::ShowBase | QTextStream::UppercaseDigits);

#define REPORT_WXAPI(API)                                                                                                                                               \
    {                                                                                                                                                                   \
        reportTextStream << #API##" : "                                                                                                                                 \
                         << injectArgs.wechat_apis.API                                                                                                                  \
                         << (injectArgs.wechat_apis.API ? "" : QString(" [<font color=\"red\">%1</font>]").arg(xstyle_manager.Translate("xstyle_meta", "UnSupported"))) \
                         << XSTYLE_REPORT_ENDL;                                                                                                                         \
    }

#define REPORT_WX_DATASTRUCT(DESC, MEMBER)                                                                                                                                                     \
    {                                                                                                                                                                                          \
        reportTextStream << "&nbsp;&nbsp;&nbsp;&nbsp;"##DESC##" : "                                                                                                                            \
                         << injectArgs.wechat_datastructure_supplement.MEMBER                                                                                                                  \
                         << (injectArgs.wechat_datastructure_supplement.MEMBER ? "" : QString(" [<font color=\"red\">%1</font>]").arg(xstyle_manager.Translate("xstyle_meta", "UnSupported"))) \
                         << XSTYLE_REPORT_ENDL;                                                                                                                                                \
    }

    REPORT_WXAPI(CheckAppSingleton);
    REPORT_WXAPI(FetchGlobalContactContextAddress);
    REPORT_WXAPI(InitWeChatContactItem);
    REPORT_WXAPI(DeinitWeChatContactItem);
    REPORT_WXAPI(FindAndDeepCopyWeChatContactItemWithWXIDWrapper);
    REPORT_WXAPI(FetchGlobalProfileContext);
    REPORT_WXAPI(HandleRawMessages);
    REPORT_WXAPI(HandleReceivedMessages);
    REPORT_WXAPI(WXSendTextMessage);
    REPORT_WXAPI(FetchGlobalSendMessageContext);
    REPORT_WXAPI(WXSendFileMessage);
    REPORT_WXAPI(CloseLoginWnd);
    REPORT_WXAPI(LogoutAndExitWeChat);
    REPORT_WXAPI(Logouted);
    REPORT_WXAPI(LogoutedByMobile);
    REPORT_WXAPI(Logined);
    REPORT_WXAPI(WeChatEventProc);
    reportTextStream << XSTYLE_REPORT_ENDL;

    reportTextStream << "DataStructure : " << XSTYLE_REPORT_ENDL;
    REPORT_WX_DATASTRUCT("Profile NickName Offset", profileItemOffset.NickName);
    REPORT_WX_DATASTRUCT("Profile NickName WeChatNumber", profileItemOffset.WeChatNumber);
    REPORT_WX_DATASTRUCT("Profile NickName WXID", profileItemOffset.Wxid);
    REPORT_WX_DATASTRUCT("Logout Event ID", logoutTriggerEventId);
    REPORT_WX_DATASTRUCT("Contact Header Item Offset", weChatContactHeaderItemOffset);
    REPORT_WX_DATASTRUCT("Contact Data Begin Offset", weChatContactDataBeginOffset);
    REPORT_WX_DATASTRUCT("Message Structure Size", weChatMessageStructureSize);

    xstyle::report(view, title, reportText);
}

//
// WeChat Status
//

void WxBoxController::UpdateClientProfile(wb_process::PID pid, const wb_wx::WeChatProfile& profile)
{
    auto client = view->wxStatusModel.get(pid);
    if (!client) {
        return;
    }

    client->logined  = profile.logined;
    client->nickname = profile.nickname.c_str();
    client->wxnumber = profile.wxnumber.c_str();
    client->wxid     = profile.wxid.c_str();
    client->update();
}

void WxBoxController::UpdateClientStatus(wb_process::PID pid) noexcept
{
    auto client = view->wxStatusModel.get(pid);
    if (!client) {
        return;
    }

    client->status = GetClientStatus(pid);
    if (client->status != WxBoxClientItemStatus::Normal) {
        client->logined = false;
    }
    if (client->status == WxBoxClientItemStatus::Independent) {
        auto it = clientInjectArgs.find(pid);
        if (it != clientInjectArgs.end()) {
            clientInjectArgs.erase(it);
        }
    }
    client->update();
}

void WxBoxController::UpdateWeChatStatus()
{
    if (view->isMinimized()) {
        return;
    }

    auto monitorInterval = statusMonitorInterval;
    if (!view->isActiveWindow()) {
        monitorInterval *= 2;
    }

    time_t timestamp = wb_process::GetCurrentTimestamp();
    if (timestamp - lastUpdateStatusTimestamp < monitorInterval) {
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

        if (clientInjectArgs.find(pid) != clientInjectArgs.end()) {
            item->fullFeatures = wb_crack::IsFullFeaturesValid(clientInjectArgs.at(pid));
        }

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
            RequestInjectArgs(message.pid);
            RequestProfile(message.pid);
            UpdateClientStatus(message.pid);
            break;

        case wxbox::WxBoxMessageType::WxBoxClientDone:
            UpdateClientStatus(message.pid);
            break;

        case wxbox::WxBoxMessageType::WxBotRequestOrResponse: {
            WxBotRequestOrResponseHandler(message);
            break;
        }
    }
}

void WxBoxController::WxBotRequestOrResponseHandler(wxbox::WxBoxMessage& message)
{
    switch (message.u.wxBotControlPacket.type()) {
        case wxbox::ControlPacketType::PROFILE_RESPONSE: {
            ProfileResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_profileresponse());
            break;
        }

        case wxbox::ControlPacketType::ALL_CONTACT_RESPONSE: {
            AllContactResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_allcontactresponse());
            break;
        }

        case wxbox::ControlPacketType::INJECT_ARGS_RESPONSE: {
            InjectArgsResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_injectargsresponse());
            break;
        }

        case wxbox::ControlPacketType::EXECUTE_PLUGIN_SCRIPT_RESPONSE: {
            ExecutePluginScriptResponseHandler(message.pid, message.u.wxBotControlPacket.mutable_executepluginscriptresponse());
            break;
        }

        case wxbox::ControlPacketType::LOG_REQUEST: {
            WxBotLogRequestHandler(message.pid, message.u.wxBotControlPacket.mutable_logrequest());
            break;
        }

        case wxbox::ControlPacketType::CLEAR_COMMAND_LOG_REQUEST: {
            view->ClearCommandResultScreen();
            break;
        }

        case wxbox::ControlPacketType::EXIT_WXBOX_REQUEST: {
            view->quit();
            break;
        }
    }
}

//
// WxBoxServer Wrapper Request Methods
//

void WxBoxController::RequestInjectArgs(wb_process::PID clientPID)
{
    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::INJECT_ARGS_REQUEST);
    PushMessageAsync(std::move(msg));
}

void WxBoxController::RequestProfile(wb_process::PID clientPID)
{
    if (clientInjectArgs.find(clientPID) != clientInjectArgs.end()) {
        auto injectArgs = clientInjectArgs.at(clientPID);
        if (!injectArgs.wechat_apis.FetchGlobalProfileContext) {
            xstyle::warning(view, "", _wctr("UnSupported this feature"));
            return;
        }
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::PROFILE_REQUEST);
    PushMessageAsync(std::move(msg));
}

void WxBoxController::RequstLogoutWeChat(wb_process::PID clientPID)
{
    if (clientInjectArgs.find(clientPID) != clientInjectArgs.end()) {
        auto injectArgs = clientInjectArgs.at(clientPID);
        if (!injectArgs.wechat_apis.WeChatEventProc) {
            xstyle::warning(view, "", _wctr("UnSupported this feature"));
            return;
        }
    }

    auto clientStatusItem = view->wxStatusModel.get(clientPID);
    if (!clientStatusItem || !clientStatusItem->logined) {
        return;
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::LOGOUT_WECHAT_REQUEST);
    PushMessageAsync(std::move(msg));
}

void WxBoxController::RequstAllContact(wb_process::PID clientPID)
{
    if (clientInjectArgs.find(clientPID) != clientInjectArgs.end()) {
        auto injectArgs = clientInjectArgs.at(clientPID);
        if (!injectArgs.wechat_apis.FetchGlobalContactContextAddress) {
            xstyle::warning(view, "", _wctr("UnSupported this feature"));
            return;
        }
    }

    auto clientStatusItem = view->wxStatusModel.get(clientPID);
    if (!clientStatusItem || !clientStatusItem->logined) {
        return;
    }

    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::ALL_CONTACT_REQUEST);
    PushMessageAsync(std::move(msg));
}

void WxBoxController::RequestExecutePluginScript(wb_process::PID clientPID, const std::string& statement)
{
    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.pid = clientPID;
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::EXECUTE_PLUGIN_SCRIPT_REQUEST);

    auto executePluginScriptRequest = msg.u.wxBoxControlPacket.mutable_executepluginscriptrequest();
    executePluginScriptRequest->set_statement(statement.c_str());

    PushMessageAsync(std::move(msg));
}

void WxBoxController::RequestChangeConfig()
{
    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxRequest);
    msg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::CHANGE_CONFIG_REQUEST);

    auto changeConfigRequest = msg.u.wxBoxControlPacket.mutable_changeconfigrequest();
    changeConfigRequest->set_avoidrevokemessage(config.wechat_avoid_revoke_message());
    changeConfigRequest->set_enablerawmessagehook(config.wechat_enable_raw_message_hook());
    changeConfigRequest->set_enablesendtextmessagehook(config.wechat_enable_send_text_message_hook());
    changeConfigRequest->set_wxboxclientreconnectinterval(config.wxbox_client_reconnect_interval());
    changeConfigRequest->set_pluginlongtasktimeout(config.plugin_long_task_timeout());
    changeConfigRequest->set_serveruri(config.wxbox_server_uri());

    for (const auto& client : clientInjectArgs) {
        msg.pid = client.first;
        PushMessageAsync(msg);
    }
}

//
// WxBoxServer Response Handler
//

void WxBoxController::InjectArgsResponseHandler(wb_process::PID clientPID, wxbox::InjectArgsResponse* response)
{
    const std::string& pInjectArgsResponseBuffer = response->args();
    if (pInjectArgsResponseBuffer.empty() || pInjectArgsResponseBuffer.size() != sizeof(wb_crack::WxBotEntryParameter)) {
        return;
    }

    wb_crack::WxBotEntryParameter injectArgs;
    std::memcpy(&injectArgs, pInjectArgsResponseBuffer.data(), sizeof(wb_crack::WxBotEntryParameter));

    auto client = view->wxStatusModel.get(clientPID);
    if (client) {
        client->fullFeatures = wb_crack::IsFullFeaturesValid(injectArgs);
        client->update();
    }

    clientInjectArgs.emplace(clientPID, std::move(injectArgs));
}

void WxBoxController::ProfileResponseHandler(wb_process::PID clientPID, wxbox::ProfileResponse* response)
{
    if (!response) {
        return;
    }

    wb_wx::WeChatProfile profile(response->logined(), response->nickname(), response->wxnumber(), response->wxid());
    UpdateClientProfile(clientPID, profile);
}

void WxBoxController::AllContactResponseHandler(wb_process::PID clientPID, wxbox::AllContactResponse* response)
{
    if (!response) {
        return;
    }

    std::vector<wb_wx::WeChatContact> contacts;
    for (auto contact : response->contacts()) {
        contacts.emplace_back(wb_wx::WeChatContact(contact.chatroom(), contact.nickname(), contact.wxnumber(), contact.wxid(), contact.remark()));
    }

    auto client = view->wxStatusModel.get(clientPID);
    if (!client) {
        return;
    }

    view->contactListDialog.DisplayContactList(client->wxnumber, contacts);
}

void WxBoxController::ExecutePluginScriptResponseHandler(wb_process::PID clientPID, wxbox::ExecutePluginScriptResponse* response)
{
    if (!response) {
        return;
    }

    view->AppendExecuteCommandResult(QString("[<font color=\"blue\">%1</font>] : %2").arg(clientPID).arg(QString(response->result().c_str()).toHtmlEscaped()));
}

void WxBoxController::WxBotLogRequestHandler(wb_process::PID clientPID, wxbox::LogRequest* logRequest)
{
    if (!logRequest) {
        return;
    }

    auto message        = logRequest->msg().c_str();
    auto escapedMessage = QString(message).toHtmlEscaped();

    switch (logRequest->level()) {
        case wxbox::WxBotLogLevel::Information: {
            WXBOT_LOG_INFO(message);
            view->AppendExecuteCommandResult(QString("[<font color=\"blue\">%1</font>] [<font color=\"blue\">info</font>] : %2").arg(clientPID).arg(escapedMessage));
            break;
        }

        case wxbox::WxBotLogLevel::Warning: {
            WXBOT_LOG_WARNING(message);
            view->AppendExecuteCommandResult(QString("[<font color=\"blue\">%1</font>] [<font color=\"orange\">warning</font>] : %2").arg(clientPID).arg(escapedMessage));
            break;
        }

        case wxbox::WxBotLogLevel::Error: {
            WXBOT_LOG_ERROR(message);
            view->AppendExecuteCommandResult(QString("[<font color=\"blue\">%1</font>] [<font color=\"red\">error</font>] : %2").arg(clientPID).arg(escapedMessage));
            break;
        }
    }
}