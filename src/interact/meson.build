interact_inc = [
    utils_inc,
    plugin_inc,
    configuration_inc
]

interact_src = [
    join_paths(interact_root, 'interact.cpp')
]

# add interact executable
executable('interact', interact_src, include_directories: interact_inc,
    dependencies: [lua_dep, frida_gum_dep, yaml_cpp_dep], cpp_args: [ignore_warnings],
    link_with: [utils_static_lib, plugin_static_lib])