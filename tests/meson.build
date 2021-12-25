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

test_exe = executable('test_all', test_src, include_directories: test_inc, dependencies: [gtest_dep, gmock_dep])
test('test_all', test_exe)