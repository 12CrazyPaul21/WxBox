#include <test_common.h>

//
// wxbox_utils
//

TEST(wxbox_utils, file)
{
    auto processPath = wxbox::util::file::GetProcessRootPath();
    EXPECT_NE("", processPath);
    spdlog::info("prcoess root path is : {}", processPath);

    auto processPathExist = wxbox::util::file::IsPathExists(processPath);
    EXPECT_EQ(true, processPathExist);
    spdlog::info("prcoess root path exist : {}", processPathExist);

    auto processPathIsDirectory = wxbox::util::file::IsDirectory(processPath);
    EXPECT_EQ(true, processPathIsDirectory);
    spdlog::info("prcoess root path is directory : {}", processPathIsDirectory);

    auto joinedPath = wxbox::util::file::JoinPath("x:\\folder", "is_a_filename");
#if WXBOX_IN_WINDOWS_OS
    EXPECT_EQ("x:\\folder\\is_a_filename", joinedPath);
#elif WXBOX_IN_MAC_OS
    EXPECT_EQ("x:/folder/is_a_filename", joinedPath);
#endif
    spdlog::info("joined path : {}", joinedPath);

    wb_file::VersionNumber versionNumber;

    //
    // valid version number
    //

    wxbox::util::file::UnwindVersionNumber("0.1.55", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 0 && versionNumber.minor == 1 && versionNumber.revision == 55 && versionNumber.build == 0);
    spdlog::info("unwind version[0.1.55] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("0.1.55.2", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 0 && versionNumber.minor == 1 && versionNumber.revision == 55 && versionNumber.build == 2);
    spdlog::info("unwind version[0.1.55.2] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("5", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 5 && versionNumber.minor == 0 && versionNumber.revision == 0 && versionNumber.build == 0);
    spdlog::info("unwind version[5] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("5.0", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 5 && versionNumber.minor == 0 && versionNumber.revision == 0 && versionNumber.build == 0);
    spdlog::info("unwind version[5.0] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("5.1.0.111", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 5 && versionNumber.minor == 1 && versionNumber.revision == 0 && versionNumber.build == 111);
    spdlog::info("unwind version[5.1.0.111] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);

    //
    // invalid version number
    //

    wxbox::util::file::UnwindVersionNumber("5.1.0.111.", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 0 && versionNumber.minor == 0 && versionNumber.revision == 0 && versionNumber.build == 0);
    spdlog::info("unwind version[5.1.0.111.] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("0.", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 0 && versionNumber.minor == 0 && versionNumber.revision == 0 && versionNumber.build == 0);
    spdlog::info("unwind version[0.] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);
    wxbox::util::file::UnwindVersionNumber("0.1.", versionNumber);
    EXPECT_EQ(true, versionNumber.major == 0 && versionNumber.minor == 0 && versionNumber.revision == 0 && versionNumber.build == 0);
    spdlog::info("unwind version[0.1.] number : {}.{}.{}.{}", versionNumber.major, versionNumber.minor, versionNumber.revision, versionNumber.build);

    //
    // compare version
    //

    wxbox::util::file::VersionNumber versionNumber1, versionNumber2, versionNumber3, versionNumber4;
    wxbox::util::file::UnwindVersionNumber("3.2.6", versionNumber1);
    wxbox::util::file::UnwindVersionNumber("3.2.6.0", versionNumber2);
    wxbox::util::file::UnwindVersionNumber("3.2.6.1", versionNumber3);
    wxbox::util::file::UnwindVersionNumber("2.2.6.1", versionNumber4);
    EXPECT_EQ(0, versionNumber1.compare(versionNumber1));
    EXPECT_EQ(0, versionNumber1.compare(versionNumber2));
    EXPECT_EQ(-1, versionNumber1.compare(versionNumber3));
    EXPECT_EQ(1, versionNumber1.compare(versionNumber4));
    EXPECT_EQ(true, versionNumber1 == versionNumber1);
    EXPECT_EQ(true, versionNumber1 == versionNumber2);
    EXPECT_EQ(false, versionNumber1 != versionNumber2);
    EXPECT_EQ(true, versionNumber1 != versionNumber3);
    EXPECT_EQ(false, versionNumber1 > versionNumber2);
    EXPECT_EQ(true, versionNumber1 >= versionNumber2);
    EXPECT_EQ(true, versionNumber1 > versionNumber4);
    EXPECT_EQ(true, versionNumber3 > versionNumber4);
    EXPECT_EQ(false, versionNumber3 < versionNumber4);
    EXPECT_EQ(true, versionNumber1 <= versionNumber2);
    EXPECT_EQ(true, versionNumber1 <= versionNumber3);
    EXPECT_EQ(false, versionNumber3 <= versionNumber4);
}

TEST(wxbox_utils, string)
{
    std::string  str1 = "from string";
    std::wstring str2 = L"from wstring";

    EXPECT_EQ(0, wb_string::ToWString(str1).compare(L"from string"));
    EXPECT_EQ(0, wb_string::ToString(str2).compare("from wstring"));
}

TEST(wxbox_utils, process)
{
    auto processLists = wxbox::util::process::GetProcessList();
    EXPECT_NE(size_t(0), processLists.size());
    spdlog::info("prcoess count : {}", processLists.size());

    // Get process info from window handle(from screen point)
    // wb_process::ProcessInfo pi;
    // wb_process::WIN_HANDLE  hWnd = wb_process::GetWindowHandleFromScreenPoint({-680, 300});
    // if (hWnd) {
    //     if (wb_process::GetProcessInfoFromWindowHandle(hWnd, pi)) {
    //     	// use pi
    // 	}
    // }
}

TEST(wxbox_utils, feature_list)
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

TEST(wxbox_utils, wx)
{
    AppConfig& config = AppConfig::singleton();

    auto wxInstallationPath = wxbox::util::wx::GetWxInstallationPath();
    EXPECT_NE("", wxInstallationPath);
    spdlog::info("wx installation path : {}", wxInstallationPath);

	auto wxModuleFolderPath = wxbox::util::wx::GetWxModuleFolderPath(wxInstallationPath);
    EXPECT_NE("", wxModuleFolderPath);
    spdlog::info("wx module folder path : {}", wxModuleFolderPath);

    auto wxInstallationPathIsValid = wxbox::util::wx::IsWxInstallationPathValid(wxInstallationPath);
    EXPECT_EQ(true, wxInstallationPathIsValid);
    spdlog::info("wx installation path is valid : {}", wxInstallationPathIsValid);

    auto wxVersion = wxbox::util::wx::GetWxVersion(wxModuleFolderPath);
    EXPECT_NE("", wxVersion);
    spdlog::info("wx version : {}", wxVersion);

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    auto                         resolveSuccess = wxbox::util::wx::ResolveWxEnvInfo(wxInstallationPath, wxModuleFolderPath, wxEnvInfo);
    EXPECT_EQ(true, resolveSuccess);
    spdlog::info("wechat environment success : {}", resolveSuccess);

    // unwind feature
    wb_feature::WxApiFeatures features;
    auto featuresPath = config.features_path();
    EXPECT_NE("", featuresPath);
    EXPECT_EQ(true, wb_feature::PreLoadFeatures(featuresPath, features));

    auto wxProcessLists = wxbox::util::wx::GetWeChatProcessList();
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

TEST(wxbox_utils, crack)
{
    AppConfig& config             = AppConfig::singleton();

    auto wxInstallationPath = wxbox::util::wx::GetWxInstallationPath();
    if (wxInstallationPath.empty()) {
        return;
    }

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::util::wx::ResolveWxEnvInfo(wxInstallationPath, wxEnvInfo)) {
        return;
    }

    // unwind feature
    wb_feature::WxApiFeatures features;
    auto                      featuresPath = config.features_path();
    EXPECT_NE("", featuresPath);
    EXPECT_EQ(true, wb_feature::PreLoadFeatures(featuresPath, features));

    // open wechat with multi boxing
    wb_crack::OpenWxWithMultiBoxingResult openResult           = {0};
    auto                                  wxMultiBoxingSuccess = wxbox::util::crack::OpenWxWithMultiBoxing(wxEnvInfo, features, &openResult);
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

        wxEnvInfo.version = "3.2.2";
        for (auto api : wb_feature::WX_HOOK_API) {
            ucpulong_t addr = features.Locate(locateTarget, wxEnvInfo.version, api);
            EXPECT_NE(ucpulong_t(0), addr);
            spdlog::info("unknwon version(v3.2.2) {} VA : 0x{:08X}", api, addr);
        }
    }

#if WXBOX_IN_WINDOWS_OS
    CloseHandle(hProcess);
#elif WXBOX_IN_MAC_OS

#endif
}

TEST(wxbox_utils, inject)
{
    auto processPath = wxbox::util::file::GetProcessRootPath();
    EXPECT_NE(true, processPath.empty());

    char            callFuncName[] = "SayHiInject";
    wb_process::PID pid            = GetCurrentProcessId();
    EXPECT_NE(wb_process::PID(0), pid);

#if WXBOX_IN_WINDOWS_OS
    char moduleName[] = "ModForInjectTest.dll";
    auto modulePath   = wxbox::util::file::JoinPath(processPath, "/ModForInjectTest/ModForInjectTest.dll");
#else
    char moduleName[] = "ModForInjectTest.so";
    auto modulePath   = wxbox::util::file::JoinPath(processPath, "/ModForInjectTest/ModForInjectTest.so");
#endif

    char                              message[] = "Inject";
    wb_inject::MethodCallingParameter parameter = wb_inject::MethodCallingParameter::BuildBufferValue(message, sizeof(message));

    EXPECT_EQ(true, wb_inject::InjectModuleToProcess(pid, modulePath, callFuncName, &parameter));
    EXPECT_EQ(true, wb_inject::UnInjectModuleFromProcess(pid, moduleName));
}

TEST(wxbox_utils, DISABLED_use_frida_to_inject)
{
    // #include <frida-core.h>

    // frida_init();
    // FridaInjector* injector = frida_injector_new();

    // GError* error = nullptr;
    // auto    e     = frida_injector_inject_library_file_sync(injector, 628036, "forhook.dll", "SayHi", "", nullptr, &error);

    // frida_injector_close_sync(injector, nullptr, nullptr);
    // frida_unref(injector);
    // frida_deinit();
}

TEST(wxbox_utils, list_file)
{
    auto utilsSrcPath = wb_file::JoinPath(wb_file::GetProcessRootPath(), "../../../src/utils");
    auto cppFileList  = wb_file::ListFilesInDirectoryWithExt(utilsSrcPath, "cpp");
    EXPECT_NE(size_t(0), cppFileList.size());
    spdlog::info("{} all cpp files : ", utilsSrcPath.c_str());
    for (auto cpp : cppFileList) {
        spdlog::info("    {}", cpp.c_str());
    }
}

TEST(wxbox_utils, DISABLED_folder_monitor)
{
    auto path = wb_file::GetProcessRootPath();
    EXPECT_EQ(true, wb_file::OpenFolderFilesChangeMonitor(path, [](wb_file::FileChangeMonitorReport report) {
                  std::stringstream ss;
                  ss << "dirpath : " << report.dirpath;

                  switch (report.type) {
                      case wb_file::FileChangeType::Added:
                          ss << "  [add] : " << report.filename << std::endl;
                          break;
                      case wb_file::FileChangeType::Removed:
                          ss << "  [remove] : " << report.filename << std::endl;
                          break;
                      case wb_file::FileChangeType::Modified:
                          ss << "  [modify] : " << report.filename << std::endl;
                          break;
                      case wb_file::FileChangeType::Renamed:
                          ss << "  [rename] : " << report.oldname << " -> " << report.filename << std::endl;
                          break;
                  }

                  std::cout << ss.str();
              }));

    WaitForPressAnyKey("<<<<< Monitor Folder, Press Any Key to Stop Monitor... >>>>>");
    wb_file::CloseFolderFilesChangeMonitor(path);
}

TEST(wxbox_utils, config)
{
    static constexpr char TEST_CONFIG_NAME[] = "test_config.yml";
    wb_config::Config     config(TEST_CONFIG_NAME);

    // load config
    EXPECT_EQ(true, config.load(TEST_CONFIG_NAME));

    config["/wxbox/foo"_conf] = 1;
    config["/wxbox/bar"_conf] = "2";
    config["/wx/list"_conf]   = std::vector<std::string>({"hello", "world"});
    config.submit();
    config.close();

    config.load();
    EXPECT_EQ(1, config[R"(/wxbox/foo)"_conf].safe_as<int>());
    EXPECT_EQ(0, config[R"(/wxbox/bar)"_conf].safe_as<std::string>().compare("2"));
    EXPECT_EQ(0, config["/wx/list"_conf][0].safe_as<std::string>().compare("hello"));
    EXPECT_EQ(0, config["/wx/list"_conf][1].safe_as<std::string>().compare("world"));

    for (auto item : config["/wx/list"_conf]) {
        spdlog::info("{}", item.safe_as<std::string>());
    }
}

TEST(wxbox_utils, app_config)
{
    static constexpr char TEST_CONFIG_NAME[] = "test_config.yml";
    AppConfig&            config             = AppConfig::singleton();

    // load config
    EXPECT_EQ(true, config.load(TEST_CONFIG_NAME));

    // print default config
    spdlog::info("default config : ");
    spdlog::info("    /wxbox/plugins_relpath : {}", config[WXBOX_PLUGINS_RELPATH_KEY].safe_as<std::string>());
    spdlog::info("    /wxbox/language : {}", config[WXBOX_LANGUAGE_KEY].safe_as<std::string>());
    spdlog::info("    /wxbox/plugins_relpath : {}", config[WXBOX_COREDUMP_PATH_KEY].safe_as<std::string>());
    spdlog::info("    /wxbox/wechat_installation_dir : {}", config[WXBOX_WECHAT_INSTALLATION_DIR_KEY].safe_as<std::string>());
    spdlog::info("    /wxbox/wechat_multi_bloxing_quota : {}", config[WXBOX_WECHAT_MULTI_BLOXING_QUOTA_KEY].safe_as<std::string>());

    config["/wxbox/foo"_conf] = 1;
    config["/wxbox/bar"_conf] = "2";
    config["/wx/list"_conf]   = std::vector<std::string>({"hello", "world"});
    config.submit();
    config.close();

    config.load();
    EXPECT_EQ(1, config[R"(/wxbox/foo)"_conf].safe_as<int>());
    EXPECT_EQ(0, config[R"(/wxbox/bar)"_conf].safe_as<std::string>().compare("2"));
}

TEST(wxbox_utils, timestamp_to_date)
{
    auto ts_ms = wb_process::GetCurrentTimestamp();
    auto ts_s  = wb_process::GetCurrentTimestamp(false);
    spdlog::info("date : {}", wb_process::TimeStampToDate<std::chrono::milliseconds>(ts_ms));
    spdlog::info("date : {}", wb_process::TimeStampToDate<std::chrono::seconds>(ts_s));
}

TEST(wxbox_utils, platform)
{
    spdlog::info("is 64 system : {}", wb_platform::Is64System());
    spdlog::info("is 64 process : {}", wb_process::Is64Process(wb_process::GetCurrentProcessHandle()));
    spdlog::info("system product description : {}", wb_platform::GetSystemVersionDescription());
    spdlog::info("cpu product description : {}", wb_platform::GetCPUProductBrandDescription());
}

TEST(wxbox_utils, module_infos)
{
    auto moduleInfos = wb_process::CollectModuleInfos(wb_process::GetCurrentProcessId());
    EXPECT_NE(size_t(0), moduleInfos.size());
}

std::string before_hook()
{
    spdlog::info("before hook");
    return "before hook";
}

std::string after_hook()
{
    spdlog::info("after hook");
    return "after hook";
}

TEST(wxbox_utils, hook)
{
    EXPECT_EQ(0, before_hook().compare("before hook"));
    wb_hook::InProcessDummyHook(before_hook, after_hook);
    EXPECT_EQ(0, before_hook().compare("after hook"));
    wb_hook::RevokeInProcessHook(before_hook);
    EXPECT_EQ(0, before_hook().compare("before hook"));

	wb_hook::InProcessHook(before_hook, after_hook);
    EXPECT_EQ(0, before_hook().compare("after hook"));
    wb_hook::RevokeInProcessHook(before_hook);
    EXPECT_EQ(0, before_hook().compare("before hook"));
}

TEST(wxbox_utils, log)
{
    AppConfig& config = AppConfig::singleton();
    config.load(AppConfig::APP_CONFIG_NAME);

    AppConfig::RegisterLogger();

    auto log_path                    = config.log_file_path();
    auto log_name                    = config.log_name();
    auto log_max_rotating_file_count = config.log_max_rotating_file_count();
    auto log_max_single_file_size    = config.log_max_single_file_size();
    auto log_auto_flush_interval_sec = config.log_auto_flush_interval_sec();
    auto log_level                   = config.log_level();
    auto log_pattern                 = config.log_pattern();

    spdlog::info(log_path);
    spdlog::info(log_name);
    spdlog::info(log_max_rotating_file_count);
    spdlog::info(log_max_single_file_size);
    spdlog::info(log_auto_flush_interval_sec);
    spdlog::info(log_level);
    spdlog::info(log_pattern);

    spdlog::debug("is a debug log");
    SPDLOG_DEBUG("is a DEBUG LOG");
    SPDLOG_TRACE("is a TRACE LOG");

    auto t1 = std::thread([]() {
        for (int i = 0; i < 100; i++) {
            spdlog::info(i);
        }
    });

    auto t2 = std::thread([]() {
        for (int i = 0; i < 100; i++) {
            spdlog::info(i);
        }
    });

    t1.join();
    t2.join();
}