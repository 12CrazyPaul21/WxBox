interact_inc = [
    utils_inc,
    plugin_inc,
    configuration_inc
]

interact_src = files([
    join_paths(interact_root, 'interact.cpp')
])

# resources
if host_os == 'windows'
    windows = import('windows')
    interact_src += windows.compile_resources(join_paths(interact_assets_root, 'interact.rc'), include_directories: configuration_inc)
elif host_os == 'darwin'
    # apple
endif

# add interact executable
executable('interact', interact_src, include_directories: interact_inc,
    install: true,
    install_dir: project_install_root,
    dependencies: [lua_dep, frida_gum_dep, yaml_cpp_dep], cpp_args: [ignore_warnings],
    link_with: [utils_static_lib, plugin_static_lib])