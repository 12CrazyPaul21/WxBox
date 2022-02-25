# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_WXBOT']

wxbot_inc = [
    utils_inc,
    configuration_inc
]

wxbot_src = files([
    join_paths(wxbot_root, 'wxbot.hpp'),
    join_paths(wxbot_root, 'wxbot.cpp'),
    join_paths(wxbot_root, 'wxbox_client.hpp'),
    join_paths(wxbot_root, 'wxbox_client.cpp'),
    join_paths(wxbot_root, 'wxbot_intercept_handler.cpp'),
    join_paths(wxbot_root, 'wxbot_wechat_api_wrapper.cpp'),
    join_paths(wxbot_root, 'wxbot_client_response_wrapper.cpp'),
    join_paths(wxbot_root, 'wxbot_plugin_api_wrapper.cpp'),
])

# resources
if host_os == 'windows'
    windows = import('windows')
    wxbot_src += windows.compile_resources(join_paths(wxbot_assets_root, 'wxbot.rc'), include_directories: configuration_inc)
elif host_os == 'darwin'
    # apple
endif

shlib = shared_library('wxbot', [wxbot_src, proto_generated],
    include_directories: wxbot_inc,
    install : true,
    install_dir: project_install_root,
    cpp_args : [lib_args, ignore_warnings, config_macro_flags],
    gnu_symbol_visibility : 'hidden',
    dependencies: [spdlog_dep, lua_dep, grpc_dep, yaml_cpp_dep, frida_gum_dep],
    link_with: [utils_static_lib, plugin_static_lib]
)

# Test wxbot
# test_exe = executable('wxbot_test', 'wxbot_test.cpp',
#     include_directories: configuration_inc,
#     link_with : shlib,
#     dependencies: [spdlog_dep],
#     cpp_args: [ignore_warnings]
# )
# test_env = environment()
# # test_env.prepend('PATH', TitanEngine_bin_root)
# test_env.prepend('PATH', grpc_bin_root)
# test('wxbot', test_exe)
