interact_inc = [
    utils_inc,
    plugin_inc,
    configuration_inc
]

interact_src = [
    join_paths(interact_root, 'interact.cpp'),
    join_paths(utils_root, 'platform.cpp'),
    join_paths(utils_root, 'process.cpp'),
    join_paths(utils_root, 'file.cpp'),
    join_paths(utils_root, 'string.cpp')
]

# add interact executable
executable('interact', interact_src, include_directories: interact_inc,
    dependencies: [lua_dep, frida_gum_dep, TitanEngine_dep, yaml_cpp_dep], cpp_args: [ignore_warnings],
    link_with: plugin_static_lib)