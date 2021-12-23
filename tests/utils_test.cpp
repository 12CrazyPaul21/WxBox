#include <test_common.h>

#include <wx.h>

TEST(wxbox_utils, wx)
{
    EXPECT_EQ("is stub", wxbox::util::GetWxInstallationPath());
}