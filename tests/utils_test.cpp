#include <test_common.h>

#include <utils/wx.h>

TEST(wxbox_utils, wx)
{
    EXPECT_NE("", wxbox::util::GetWxInstallationPath());
}