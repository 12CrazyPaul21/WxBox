project('WxBox', ['c', 'cpp'],
    version : '0.1.0',
    default_options : ['warning_level=3',
                        'cpp_std=c++14']
)

host_os = host_machine.system()
build_type = get_option('buildtype')

proj_root = meson.current_source_dir()
source_root = join_paths(proj_root, 'src')
tests_root = join_paths(proj_root, 'tests')
assets_root = join_paths(proj_root, 'assets')

wxbox_root = join_paths(source_root, 'wxbox')
wxbot_root = join_paths(source_root, 'wxbot')
utils_root = join_paths(source_root, 'utils')

#
# config.h
#

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
if host_os == 'windows'
    conf_data.set('os', 'WXBOX_WINDOWS_OS')
elif host_os == 'darwin'
    conf_data.set('os', 'WXBOX_MAC_OS')
else
    conf_data.set('os', 'WXBOX_UNSUPPORT_OS')
endif
conf_data.set('wxbox_module_version', get_option('wxbox_module_version'))
conf_data.set('wxbot_module_version', get_option('wxbot_module_version'))
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)
configuration_inc = include_directories('.')

#
# clang_format
#

clang_format = find_program('clang-format', required: false)
if not clang_format.found() and host_os == 'windows'
    clang_format = find_program(['clang-format', join_paths(meson.current_source_dir(), 'wintools/bin/clang-format')], required: true)
    meson.override_find_program('clang-format', clang_format)
endif

#
# handle dependency
#

# qt
qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'], main: true)

# spdlog
spdlog_dep = dependency('spdlog', required: false)
if not spdlog_dep.found()
    cmake = import('cmake')
    spdlog_proj_opt_var = cmake.subproject_options()
    spdlog_proj_opt_var.add_cmake_defines({'CMAKE_BUILD_TYPE': build_type})
    spdlog_proj = cmake.subproject('spdlog', options: spdlog_proj_opt_var)
    spdlog_dep = spdlog_proj.dependency('spdlog')
endif

# gRPC(contain protobuf)
grpc_dep = dependency('grpc', required: false)
if not grpc_dep.found()
    if build_type == 'debug'
        grpc_proj = subproject('grpc_debug')
    else
        grpc_proj = subproject('grpc')
    endif
    grpc_dep = grpc_proj.get_variable('grpc_dep')
endif

# frida-gum
frida_gum_dep = dependency('frida-gum', required: false)
if not frida_gum_dep.found()
    frida_gum_proj = subproject('frida-gum')
    frida_gum_dep = frida_gum_proj.get_variable('frida_gum_dep')
endif

# lua(only for WxBot)
lua_proj = subproject('lua')
lua_dep = lua_proj.get_variable('lua_dep')

# yaml-cpp
yaml_cpp_dep = dependency('yaml-cpp', required: false)
if not yaml_cpp_dep.found()
    cmake = import('cmake')
    yaml_cpp_proj_opt_var = cmake.subproject_options()
    yaml_cpp_proj_opt_var.add_cmake_defines({
        'CMAKE_BUILD_TYPE': build_type,
        'CMAKE_POSITION_INDEPENDENT_CODE': 'ON',
        'YAML_CPP_BUILD_TOOLS': 'OFF',
        'YAML_CPP_BUILD_TESTS': 'OFF',
        'YAML_CPP_BUILD_CONTRIB': 'OFF',
        'BUILD_SHARED_LIBS': 'OFF'})
    yaml_cpp_proj = cmake.subproject('yaml-cpp', options: yaml_cpp_proj_opt_var)
    yaml_cpp_dep = declare_dependency(dependencies: [yaml_cpp_proj.dependency('yaml-cpp')],
                            compile_args: '/DYAML_CPP_STATIC_DEFINE')
endif

# TitanEngine
TitanEngine_proj = subproject('TitanEngine')
TitanEngine_dep = TitanEngine_proj.get_variable('TitanEngine_dep')
TitanEngine_bin_root = TitanEngine_proj.get_variable('TitanEngine_bin_root')

#
# utils
#

utils_inc = [
    source_root
]

utils_src = [
    join_paths(utils_root, 'common.h'),
    join_paths(utils_root, 'process.h'),
    join_paths(utils_root, 'process.cpp'),
    join_paths(utils_root, 'wx.h'),
    join_paths(utils_root, 'wx.cpp'),
    join_paths(utils_root, 'file.h'),
    join_paths(utils_root, 'file.cpp'),
    join_paths(utils_root, 'string.h'),
    join_paths(utils_root, 'string.cpp'),
    join_paths(utils_root, 'memory.h'),
    join_paths(utils_root, 'memory.cpp'),
    join_paths(utils_root, 'feature.h'),
    join_paths(utils_root, 'feature.cpp'),
    join_paths(utils_root, 'config.hpp')
]

#
# WxBox
#

subdir('src/wxbox')


#
# WxBot
#

subdir('src/wxbot')

#
# tests
#

subdir('tests')

#
# add clang-format target
#

format_command = 'scripts/pretty_format'
if host_os == 'windows'
    format_command += '.bat'
endif
format_target = run_target('pretty_format', command: format_command)

#
# install与pack【这个留到之后补充】
#
 
# note: 处理qt依赖，在Windows下使用windeployqt，Mac OS下使用macdeployqt