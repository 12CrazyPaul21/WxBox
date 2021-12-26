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
    auto processPath = wxbox::util::file::GetWxBoxRootPath();
    EXPECT_NE("", processPath);
    spdlog::info("prcoess path is : {}", processPath);
}