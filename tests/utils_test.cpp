#include <test_common.h>
#include <utils/common.h>

TEST(wxbox_utils, wx)
{
    EXPECT_NE("", wxbox::util::wx::GetWxInstallationPath());
}