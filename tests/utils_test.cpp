#include <test_common.h>
#include <utils/common.h>
#include <spdlog/spdlog.h>

TEST(wxbox_utils, wx)
{
    auto wxInstallationPath = wxbox::util::wx::GetWxInstallationPath();
    EXPECT_NE("", wxInstallationPath);
    spdlog::info("wx installation path : {}", wxInstallationPath);
}

TEST(wbox_utils, file)
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