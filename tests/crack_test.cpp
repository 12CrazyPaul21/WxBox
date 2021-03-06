#include <test_common.h>

//
// wxbox_crack
//

TEST(wxbox_crack, feature_list)
{
    AppConfig& config = AppConfig::singleton();

    auto featuresPath = config.features_path();
    EXPECT_NE("", featuresPath);
    spdlog::info("features folder path : {}", featuresPath);

    wb_feature::WxApiFeatures features;
    EXPECT_EQ(true, wb_feature::PreLoadFeatures(featuresPath, features));
    PrintWxApiFeatures(features);

    wb_feature::WxAbsoluteHookInfo absoluteHookInfo;
    EXPECT_EQ(true, features.GetAbsoluteHookInfo("3.4.5.27", absoluteHookInfo));
    EXPECT_NE(ucpulong_t(0), features.GetAbsoluteHookPointRVA("3.4.5.27", "CheckAppSingleton"));
    PrintWxAbsoluteHookInfo(features);

    wb_feature::WxHookPointFeatures wxHookPointFeatures;
    EXPECT_EQ(true, features.GetHookPointFeatures("3.4.5.27", wxHookPointFeatures));
    wb_feature::HookPointFeatureInfo hookPointFeatureInfo;
    EXPECT_EQ(true, features.GetHookPointFeature("3.4.5.27", "CheckAppSingleton", hookPointFeatureInfo));
    PrintWxHookPointFeatures(features);

    EXPECT_EQ(true, features.GetSimilarHookPointFeatures("3.4.5.28", wxHookPointFeatures));
    EXPECT_EQ(true, features.GetSimilarHookPointFeatures("1.5.5.28", wxHookPointFeatures));
    EXPECT_EQ(true, features.GetSimilarHookPointFeature("1.5.5.208", "CheckAppSingleton", hookPointFeatureInfo));
}

TEST(wxbox_crack, wx)
{
    AppConfig& config = AppConfig::singleton();

    auto wxInstallationPath = wxbox::crack::wx::GetWxInstallationPath();
    EXPECT_NE("", wxInstallationPath);
    spdlog::info("wx installation path : {}", wxInstallationPath);

    auto wxModuleFolderPath = wxbox::crack::wx::GetWxModuleFolderPath(wxInstallationPath);
    EXPECT_NE("", wxModuleFolderPath);
    spdlog::info("wx module folder path : {}", wxModuleFolderPath);

    auto wxInstallationPathIsValid = wxbox::crack::wx::IsWxInstallationPathValid(wxInstallationPath);
    EXPECT_EQ(true, wxInstallationPathIsValid);
    spdlog::info("wx installation path is valid : {}", wxInstallationPathIsValid);

    auto wxVersion = wxbox::crack::wx::GetWxVersion(wxModuleFolderPath);
    EXPECT_NE("", wxVersion);
    spdlog::info("wx version : {}", wxVersion);

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    auto                         resolveSuccess = wxbox::crack::wx::ResolveWxEnvInfo(wxInstallationPath, wxModuleFolderPath, wxEnvInfo);
    EXPECT_EQ(true, resolveSuccess);
    spdlog::info("wechat environment success : {}", resolveSuccess);

    // unwind feature
    wb_feature::WxApiFeatures features;
    auto                      featuresPath = config.features_path();
    EXPECT_NE("", featuresPath);
    EXPECT_EQ(true, wb_feature::PreLoadFeatures(featuresPath, features));

    auto wxProcessLists = wxbox::crack::wx::GetWeChatProcessList();
    spdlog::info("wechat prcoess count : {}", wxProcessLists.size());
    for (auto pi : wxProcessLists) {
        spdlog::info("wechat prcoess(pid:{}) ", pi.pid);
        spdlog::info("    execute file abspath : {}", pi.abspath);
        spdlog::info("    execute filename : {}", pi.filename);
        spdlog::info("    execute dirpath : {}", pi.dirpath);

        wb_feature::WxAPIHookPointVACollection vaCollection;
        auto                                   valid = features.Collect(pi, vaCollection);
        spdlog::info("    can hook : {}", valid);
        if (!valid) {
            continue;
        }

        for (auto api : wb_feature::WX_HOOK_API) {
            spdlog::info("        {} VA : 0x{:08X}", api, vaCollection.get(api));
        }
    }
}

TEST(wxbox_crack, locate_hook_point)
{
    AppConfig& config = AppConfig::singleton();

    auto wxInstallationPath = wxbox::crack::wx::GetWxInstallationPath();
    if (wxInstallationPath.empty()) {
        return;
    }

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::crack::wx::ResolveWxEnvInfo(wxInstallationPath, wxEnvInfo)) {
        return;
    }

    // unwind feature
    wb_feature::WxApiFeatures features;
    auto                      featuresPath = config.features_path();
    EXPECT_NE("", featuresPath);
    EXPECT_EQ(true, wb_feature::PreLoadFeatures(featuresPath, features));

    // open wechat with multi boxing
    wb_crack::OpenWxWithMultiBoxingResult openResult           = {0};
    auto                                  wxMultiBoxingSuccess = wxbox::crack::OpenWxWithMultiBoxing(wxEnvInfo, features, &openResult);
    EXPECT_EQ(true, wxMultiBoxingSuccess);
    spdlog::info("wx multi boxing : {}", wxMultiBoxingSuccess);
    if (wxMultiBoxingSuccess) {
        spdlog::info("wechat new process pid : {}", openResult.pid);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // get module info
    wb_process::ModuleInfo mi;
    auto                   getModuleInfoSuccess = wxbox::util::process::GetModuleInfo(openResult.pid, WX_WE_CHAT_CORE_MODULE, mi);
    EXPECT_EQ(true, getModuleInfoSuccess);
    EXPECT_EQ((ucpulong_t)openResult.pModuleBaseAddr, (ucpulong_t)mi.pModuleBaseAddr);
    EXPECT_EQ((ucpulong_t)openResult.uModuleSize, (ucpulong_t)mi.uModuleSize);
    spdlog::info("get module info : {}", getModuleInfoSuccess);

    //
    // locate all api va
    //

    wb_process::PROCESS_HANDLE hProcess = NULL;

#if WXBOX_IN_WINDOWS_OS
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, openResult.pid);
#elif WXBOX_IN_MAC_OS

#endif

    if (hProcess) {
        //
        // known wechat version
        //

        wb_feature::LocateTarget locateTarget = {hProcess, openResult.pModuleBaseAddr, openResult.uModuleSize};
        for (auto api : wb_feature::WX_HOOK_API) {
            ucpulong_t addr = features.Locate(locateTarget, wxEnvInfo.version, api);
            EXPECT_NE(ucpulong_t(0), addr);
            spdlog::info("{} VA : 0x{:08X}", api, addr);
        }

        //
        // unknwon wechat version
        //

        wxEnvInfo.version = "3.5.2";
        for (auto api : wb_feature::WX_HOOK_API) {
            ucpulong_t addr = features.Locate(locateTarget, wxEnvInfo.version, api);
            EXPECT_NE(ucpulong_t(0), addr);
            spdlog::info("unknwon version(v3.5.2) {} VA : 0x{:08X}", api, addr);
        }
    }

#if WXBOX_IN_WINDOWS_OS
    CloseHandle(hProcess);
#elif WXBOX_IN_MAC_OS

#endif
}

TEST(wxbox_crack, inject_wxbot_module)
{
    auto wxProcessList = wb_wx::GetWeChatProcessList();
    if (wxProcessList.empty()) {
        return;
    }

    auto wxPid       = wxProcessList.front().pid;
    auto processPath = wxbox::util::file::GetProcessRootPath();
    EXPECT_NE(true, processPath.empty());

    std::string moduleFolderPath = wxbox::util::file::JoinPath(processPath, "/../src/wxbot/");
    wb_inject::AddModuleSearchPath(wxPid, moduleFolderPath);

#if WXBOX_IN_WINDOWS_OS
    char moduleName[] = "wxbot.dll";
#else
    char moduleName[] = "wxbot.so";
#endif

    char                              callFuncName[] = "WxBotMain";
    char                              message[]      = "Hello";
    wb_inject::MethodCallingParameter parameter      = wb_inject::MethodCallingParameter::BuildBufferValue(message, sizeof(message));

    EXPECT_EQ(true, wb_inject::InjectModuleToProcess(wxPid, moduleName, callFuncName, &parameter));
    EXPECT_EQ(true, wb_inject::UnInjectModuleFromProcess(wxPid, moduleName));
}

TEST(wxbox_crack, run_wx_and_inject_wxbot)
{
    AppConfig& config = AppConfig::singleton();

    // resolve wechat environment info
    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::crack::wx::ResolveWxEnvInfo(wxEnvInfo)) {
        return;
    }

    // unwind feature
    wb_feature::WxApiFeatures features;
    if (!wb_feature::PreLoadFeatures(config.features_path(), features)) {
        return;
    }

    // open and wechat with multi boxing
    wb_crack::OpenWxWithMultiBoxingResult openResult = {0};
    if (!wxbox::crack::OpenWxWithMultiBoxing(wxEnvInfo, features, &openResult, true)) {
        return;
    }
    spdlog::info("wechat new process pid : {}", openResult.pid);

	// get process info
    wb_process::AutoProcessHandle aphandle = wb_process::OpenProcessAutoHandle(openResult.pid);
    if (!aphandle.valid()) {
        return;
    }

	// collect hook point
    wb_feature::LocateTarget               locateTarget = {aphandle.hProcess, openResult.pModuleBaseAddr, openResult.uModuleSize};
    wb_feature::WxAPIHookPointVACollection vaCollection;
    EXPECT_EQ(true, features.Collect(locateTarget, wxEnvInfo.version, vaCollection));
    aphandle.close();

	// deattach
    wb_crack::DeAttachWxProcess(openResult.pid);

	//
	// inject wxbot
	//

	auto wxboxRoot = wb_file::GetProcessRootPath();
	auto wxbotRoot = config.wxbot_root_path();

	// wxbot entry parameter
	wb_crack::WxBotEntryParameter wxbotEntryParameter;
    std::memset(&wxbotEntryParameter, 0, sizeof(wxbotEntryParameter));
    wxbotEntryParameter.wxbox_pid = wb_process::GetCurrentProcessId();
    strcpy_s(wxbotEntryParameter.wxbox_root, sizeof(wxbotEntryParameter.wxbox_root), wxboxRoot.data());
    strcpy_s(wxbotEntryParameter.wxbot_root, sizeof(wxbotEntryParameter.wxbot_root), wxbotRoot.data());
    EXPECT_EQ(true, wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis));
    EXPECT_EQ(true, wb_crack::VerifyWxApis(wxbotEntryParameter.wechat_apis));

	// inject
    EXPECT_EQ(true, wb_crack::InjectWxBot(openResult.pid, wxbotEntryParameter));

	//
	// uninject
	//

	EXPECT_EQ(true, wb_crack::IsWxBotInjected(openResult.pid));
    EXPECT_EQ(true, wb_crack::UnInjectWxBot(openResult.pid));
}

TEST(wxbox_crack, inject_wxbot_to_wxprocess)
{
    AppConfig& config = AppConfig::singleton();

    // resolve wechat environment info
    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::crack::wx::ResolveWxEnvInfo(wxEnvInfo)) {
        return;
    }

    // unwind feature
    wb_feature::WxApiFeatures features;
    if (!wb_feature::PreLoadFeatures(config.features_path(), features)) {
        return;
    }

    // open and wechat with multi boxing
    wb_crack::OpenWxWithMultiBoxingResult openResult = {0};
    if (!wxbox::crack::OpenWxWithMultiBoxing(wxEnvInfo, features, &openResult)) {
        return;
    }
    spdlog::info("wechat process pid : {}", openResult.pid);
    std::this_thread::sleep_for(std::chrono::seconds(2));

	// attach process
    EXPECT_EQ(true, wb_crack::AttachWxProcess(openResult.pid));

    // get process info
    wb_process::AutoProcessHandle aphandle = wb_process::OpenProcessAutoHandle(openResult.pid);
    if (!aphandle.valid()) {
        return;
    }

    // collect hook point
    wb_feature::LocateTarget               locateTarget = {aphandle.hProcess, openResult.pModuleBaseAddr, openResult.uModuleSize};
    wb_feature::WxAPIHookPointVACollection vaCollection;
    EXPECT_EQ(true, features.Collect(locateTarget, wxEnvInfo.version, vaCollection));
    aphandle.close();

    // deattach
    wb_crack::DeAttachWxProcess(openResult.pid);

    //
    // inject wxbot
    //

    auto wxboxRoot = wb_file::GetProcessRootPath();
    auto wxbotRoot = config.wxbot_root_path();

    // wxbot entry parameter
    wb_crack::WxBotEntryParameter wxbotEntryParameter;
    std::memset(&wxbotEntryParameter, 0, sizeof(wxbotEntryParameter));
    wxbotEntryParameter.wxbox_pid = wb_process::GetCurrentProcessId();
    strcpy_s(wxbotEntryParameter.wxbox_root, sizeof(wxbotEntryParameter.wxbox_root), wxboxRoot.data());
    strcpy_s(wxbotEntryParameter.wxbot_root, sizeof(wxbotEntryParameter.wxbot_root), wxbotRoot.data());
    EXPECT_EQ(true, wb_crack::GenerateWxApis(vaCollection, wxbotEntryParameter.wechat_apis));
    EXPECT_EQ(true, wb_crack::VerifyWxApis(wxbotEntryParameter.wechat_apis));

    // inject
    EXPECT_EQ(true, wb_crack::InjectWxBot(openResult.pid, wxbotEntryParameter));

    //
    // uninject
    //

    EXPECT_EQ(true, wb_crack::IsWxBotInjected(openResult.pid));
    EXPECT_EQ(true, wb_crack::UnInjectWxBot(openResult.pid));
}