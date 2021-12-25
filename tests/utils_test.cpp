#include <test_common.h>

#include <wx.h>

TEST(wxbox_utils, wx)
{
    EXPECT_NE("", wxbox::util::GetWxInstallationPath());
}