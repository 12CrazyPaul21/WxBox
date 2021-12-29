#include <test_common.h>
#include <utils/common.h>
#include <spdlog/spdlog.h>

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
	auto resolveSuccess = wxbox::util::wx::ResolveWxEnvInfo(wxInstallationPath, &wxEnvInfo);
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