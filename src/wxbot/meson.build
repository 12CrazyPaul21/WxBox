# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_WXBOT']

wxbot_inc = [
    utils_inc,
    configuration_inc
]

wxbot_src = [
    join_paths(wxbot_root, 'wxbot.hpp'),
    join_paths(wxbot_root, 'wxbot.cpp'),
    join_paths(wxbot_root, 'wxbox_client.hpp'),
    join_paths(wxbot_root, 'wxbox_client.cpp'),
    utils_src
]

shlib = shared_library('wxbot', [wxbot_src, proto_generated],
    include_directories: wxbot_inc,
    install : true,
    cpp_args : [lib_args, ignore_warnings],
    gnu_symbol_visibility : 'hidden',
    dependencies: [spdlog_dep, frida_gum_dep, TitanEngine_dep, lua_dep, grpc_dep, yaml_cpp_dep]
)

# Test wxbot
test_exe = executable('wxbot_test', 'wxbot_test.cpp',
    include_directories: configuration_inc,
    link_with : shlib,
    dependencies: [spdlog_dep],
    cpp_args: [ignore_warnings])
test_env = environment()
test_env.prepend('PATH', TitanEngine_bin_root)
test_env.prepend('PATH', grpc_bin_root)
test('wxbot', test_exe)

# Make this library usable as a Meson subproject.
wxbot_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with : shlib)

# Make this library usable from the system's
# package manager.
install_headers('wxbot.hpp', subdir : 'wxbot')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    name : 'WxBot',
    filebase : 'wxbot',
    description : 'Wx Robot',
    subdirs : 'wxbot',
    libraries : shlib,
    version : '0.1.0',
)
