gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')
gmock_dep = gtest_proj.get_variable('gmock_dep')

test_inc = [
    utils_inc,
    configuration_inc
]

test_src = [
    join_paths(tests_root, 'main.cpp'),
    join_paths(tests_root, 'utils_test.cpp'),
    utils_src
]

test_exe = executable('test_all', test_src, include_directories: test_inc,
    dependencies: [gtest_dep, gmock_dep, frida_gum_dep, TitanEngine_dep, spdlog_dep, yaml_cpp_dep],
    build_rpath: TitanEngine_bin_root)

test_env = environment()
test_env.prepend('PATH', TitanEngine_bin_root)
test('test_all', test_exe, env: test_env)