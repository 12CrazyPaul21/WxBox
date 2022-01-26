gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')
gmock_dep = gtest_proj.get_variable('gmock_dep')

test_options = []
if get_option('disable_wxbox_utils_test')
    test_options += '-DDISABLE_WXBOX_UTILS_TEST'
endif
if get_option('disable_wxbox_plugin_test')
    test_options += '-DDISABLE_WXBOX_PLUGIN_TEST'
endif

test_inc = [
    utils_inc,
    plugin_inc,
    configuration_inc
]

test_src = [
    join_paths(tests_root, 'main.cpp'),
    join_paths(tests_root, 'utils_test.cpp'),
    join_paths(tests_root, 'plugin_test.cpp'),
    join_paths(wxbox_root, 'app_config.hpp')
]

test_exe = executable('test_all', test_src, include_directories: test_inc,
    dependencies: [gtest_dep, gmock_dep, frida_gum_dep, spdlog_dep, yaml_cpp_dep, lua_dep],
    # build_rpath: TitanEngine_bin_root,
    cpp_args: [ignore_warnings, config_macro_flags, test_options],
    link_with: [utils_static_lib, plugin_static_lib]
)

test_env = environment()
# test_env.prepend('PATH', TitanEngine_bin_root)
test('test_all', test_exe, env: test_env)

subdir('ModForInjectTest')