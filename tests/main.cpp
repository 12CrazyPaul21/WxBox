#include <test_common.h>
#include <sstream>

int main(int argc, char* argv[])
{
    std::stringstream filters;

#ifdef WXBOX_IN_WINDOWS_OS
    ::SetConsoleOutputCP(65001);
#endif

#define ADD_DISABLE_FILTER(PACKAGE_NAME)                                        \
    {                                                                           \
        filters << (filters.str().empty() ? "-" : ":") << PACKAGE_NAME << ".*"; \
    }

    testing::InitGoogleTest(&argc, argv);
#ifdef DISABLE_WXBOX_UTILS_TEST
    ADD_DISABLE_FILTER("wxbox_utils");
#endif
#ifdef DISABLE_WXBOX_PLUGIN_TEST
    ADD_DISABLE_FILTER("wxbox_plugin");
#endif

    ::testing::GTEST_FLAG(filter) = filters.str().c_str();
    return RUN_ALL_TESTS();
}