#include <spdlog/spdlog.h>
#include <test_common.h>
#include <utils/common.h>

TEST(wxbox_utils, wx)
{
    auto wxInstallationPath = wxbox::util::wx::GetWxInstallationPath();
    EXPECT_NE("", wxInstallationPath);
    spdlog::info("wx installation path : {}", wxInstallationPath);

    auto wxInstallationPathIsValid = wxbox::util::wx::IsWxInstallationPathValid(wxInstallationPath);
    EXPECT_EQ(true, wxInstallationPathIsValid);
    spdlog::info("wx installation path is valid : {}", wxInstallationPathIsValid);

    auto wxVersion = wxbox::util::wx::GetWxVersion(wxInstallationPath);
    EXPECT_NE("", wxVersion);
    spdlog::info("wx version : {}", wxVersion);

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    auto                         resolveSuccess = wxbox::util::wx::ResolveWxEnvInfo(wxInstallationPath, &wxEnvInfo);
    EXPECT_EQ(true, resolveSuccess);
    spdlog::info("wechat environment success : {}", resolveSuccess);

    auto wxMultiBoxingSuccess = wxbox::util::wx::OpenWxWithMultiBoxing(wxEnvInfo);
    EXPECT_EQ(true, wxMultiBoxingSuccess);
    spdlog::info("wx multi boxing : {}", wxMultiBoxingSuccess);
}

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

TEST(wxbox_utils, feature)
{
    auto processPath = wxbox::util::file::GetProcessRootPath();
    EXPECT_NE("", processPath);

    auto featConfPath = wxbox::util::file::JoinPath(processPath, "../../../conf/features.yml");
    EXPECT_NE("", processPath);
    spdlog::info("feature conf path : {}", featConfPath);

    wb_feature::WxApiHookInfo wxApiHookInfo;
    auto                      unwindSuccess = wxbox::util::feature::UnwindFeatureConf(featConfPath, wxApiHookInfo);
    EXPECT_EQ(true, unwindSuccess);
    spdlog::info("feature conf unwind success : {}", unwindSuccess);
    if (unwindSuccess) {
        spdlog::info("WxApiHookInfo platform : {}", wxApiHookInfo.platform);
        spdlog::info("WxApiHookInfo featureFileAbsPath : {}", wxApiHookInfo.featureFileAbsPath);

        for (auto absoluteHookInfoPair : wxApiHookInfo.mapWxAbsoluteHookInfo) {
            std::string                    wxVersion        = absoluteHookInfoPair.first;
            wb_feature::WxAbsoluteHookInfo absoluteHookInfo = absoluteHookInfoPair.second;

            spdlog::info("WxApiHookInfo absoluteHookInfo wechat version : {}", wxVersion);
            for (auto api : wb_feature::WX_HOOK_API) {
                spdlog::info("    {} RVA : 0x{:08X}", api, absoluteHookInfo.GetApiRva(api));
            }
        }

        for (auto wxHookPointFeaturesPair : wxApiHookInfo.mapWxHookPointFeatures) {
            std::string                      wxVersion         = wxHookPointFeaturesPair.first;
            wb_feature::WxHookPointFeatures  hookPointFeatures = wxHookPointFeaturesPair.second;
            wb_feature::HookPointFeatureInfo hookFeatureInfo;

            spdlog::info("WxApiHookInfo hookPointFeatures wechat version : {}", wxVersion);
            for (auto api : wb_feature::WX_HOOK_API) {
                spdlog::info("    {} hook feature :", api);
                if (!hookPointFeatures.GetApiHookFeature(api, hookFeatureInfo)) {
                    continue;
                }

                spdlog::info("        ScanType : {}", hookFeatureInfo.scanType);
                if (!hookFeatureInfo.scanType.compare("ref")) {
                    if (hookFeatureInfo.refFeatureStream.size()) {
                        spdlog::info("        RefFeatureStream :");
                        PrintUInt8Vector(hookFeatureInfo.refFeatureStream);
                    }

                    if (hookFeatureInfo.refBackExtralInstruction.size()) {
                        spdlog::info("        RefBackExtralInstruction :");
                        PrintUInt8Vector(hookFeatureInfo.refBackExtralInstruction);
                    }

                    if (hookFeatureInfo.refFrontExtralInstruction.size()) {
                        spdlog::info("        RefFrontExtralInstruction :");
                        PrintUInt8Vector(hookFeatureInfo.refFrontExtralInstruction);
                    }
                }
                else if (!hookFeatureInfo.scanType.compare("multiPushRef")) {
                    if (hookFeatureInfo.pushInstruction.size()) {
                        spdlog::info("        PushInstruction :");
                        PrintUInt8Vector(hookFeatureInfo.pushInstruction);
                    }

                    if (hookFeatureInfo.refFeatureStreams.size()) {
                        spdlog::info("        RefFeatureStreams :");
                        for (auto refFeatureStream : hookFeatureInfo.refFeatureStreams) {
                            PrintUInt8Vector(refFeatureStream);
                        }
                    }
                }
                else if (!hookFeatureInfo.scanType.compare("instruction")) {
                    if (hookFeatureInfo.instructionFeatureStream.size()) {
                        spdlog::info("        InstructionFeatureStream :");
                        PrintUInt8Vector(hookFeatureInfo.instructionFeatureStream);
                    }
                }

                spdlog::info("        LocateAction : {}", hookFeatureInfo.locateAction);
                if (hookFeatureInfo.locateActionFeatureStream.size()) {
                    spdlog::info("        LocateActionFeatureStream :");
                    PrintUInt8Vector(hookFeatureInfo.locateActionFeatureStream);
                }
                spdlog::info("        HookPointOffset : {}", hookFeatureInfo.hookPointOffset);
                if (!hookFeatureInfo.locateAction.compare("backThenFront")) {
                    if (hookFeatureInfo.thenLocateActionFeatureStream.size()) {
                        spdlog::info("        ThenLocateActionFeatureStream :");
                        PrintUInt8Vector(hookFeatureInfo.thenLocateActionFeatureStream);
                    }
                }
                else if (!hookFeatureInfo.locateAction.compare("backMultiTimes")) {
                    spdlog::info("        LocateActionExecuteTimes : {}", hookFeatureInfo.locateActionExecuteTimes);
                }
            }
        }
    }
}