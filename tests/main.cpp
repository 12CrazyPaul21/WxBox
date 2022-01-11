#include <test_common.h>

//#define DISABLE_WXBOX_UTILS_TEST

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
#ifdef DISABLE_WXBOX_UTILS_TEST
    ::testing::GTEST_FLAG(filter) = "-wxbox_utils.*";
#endif
    return RUN_ALL_TESTS();
}