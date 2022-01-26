project('WxBox', ['c', 'cpp'],
    version : '0.1.0',
    default_options : ['warning_level=3',
                        'cpp_std=c++17',
                        'backend_startup_project=wxbox']
)

host_os = host_machine.system()
host_cpu_family = host_machine.cpu_family()
build_type = get_option('buildtype')

proj_root = meson.current_source_dir()
source_root = join_paths(proj_root, 'src')
tests_root = join_paths(proj_root, 'tests')
assets_root = join_paths(proj_root, 'assets')
crashdumper_assets_root = join_paths(assets_root, 'crashdumper')

wxbox_root = join_paths(source_root, 'wxbox')
wxbot_root = join_paths(source_root, 'wxbot')
utils_root = join_paths(source_root, 'utils')
crashdumper_root = join_paths(source_root, 'crashdumper')
plugin_root = join_paths(source_root, 'plugin')
interact_root = join_paths(source_root, 'interact')

#
# compiler
#

ignore_warnings = ['/wd4125', '/wd4702', '/wd4505']

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

if host_machine.endian() == 'big'
    conf_data.set('endian', 'WXBOX_CPU_BIG_ENDIAN')
else
    conf_data.set('endian', 'WXBOX_CPU_LITTLE_ENDIAN')
endif

if host_cpu_family == 'x86'
    conf_data.set('cpu_family', 'WXBOX_CPU_X86')
elif host_cpu_family == 'x86_64'
    conf_data.set('cpu_family', 'WXBOX_CPU_X86_64')
else
    conf_data.set('cpu_family', 'WXBOX_CPU_UNSUPPORT')
endif

conf_data.set('wxbox_module_version', get_option('wxbox_module_version'))
conf_data.set('wxbot_module_version', get_option('wxbot_module_version'))
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)
configuration_inc = include_directories('.')

#
# collect default wxbox config
#

all_default_config = {
    '/wxbox/language': true,
    '/wxbox/i18n/path': true,
    '/wxbox/plugins_relpath': true,
    '/wxbox/coredump_path': true,
    '/wxbox/coredump_prefix': true,
    '/wxbox/crashdumper': true,
    '/wxbox/log/path': true,
    '/wxbox/log/basename': true,
    '/wxbox/log/max_rotating_file_count': false,
    '/wxbox/log/max_single_file_size': false,
    '/wxbox/log/auto_flush_interval_sec': false,
    '/wxbox/wechat_installation_dir': true,
    '/wxbox/wechat_multi_bloxing_quota': false
}

config_macro_flags = []
foreach config_key, quotation : all_default_config
    real_key = config_key.strip('/').replace('/', '_')
    option_key = 'default_' + real_key
    config_value = get_option(option_key)

    config_key_macro = real_key.to_upper()
    config_value_macro = option_key.to_upper() + '_VALUE'

    if quotation
        config_value = '"' + config_value + '"'
    endif

    config_macro_flags += '-D@0@="@1@"'.format(config_key_macro, config_key)
    config_macro_flags += '-D@0@=@1@'.format(config_value_macro, config_value)
endforeach

message('wxbox default config macro : ')
message(config_macro_flags)

#
# clang_format
#

clang_format = find_program('clang-format', required: false)
if not clang_format.found() and host_os == 'windows'
    clang_format = find_program(['clang-format', join_paths(meson.current_source_dir(), 'wintools/bin/clang-format')], required: true)
    meson.override_find_program('clang-format', clang_format)
endif

#
# protoc
#

protoc = find_program('protoc', required: false)
if not protoc.found() and host_os == 'windows'
    protoc = find_program(['protoc', join_paths(meson.current_source_dir(), 'wintools/bin/protoc')], required: true)
endif
if not protoc.found()
    error('protoc is required...')
endif

#
# grpc_cpp_plugin
#

grpc_cpp_plugin = find_program('grpc_cpp_plugin', required: false)
if not grpc_cpp_plugin.found() and host_os == 'windows'
    grpc_cpp_plugin = find_program(['grpc_cpp_plugin', join_paths(meson.current_source_dir(), 'wintools/bin/grpc_cpp_plugin')], required: true)
endif
if not grpc_cpp_plugin.found()
    error('grpc_cpp_plugin is required...')
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
grpc_bin_root = ''
if not grpc_dep.found() and host_os == 'windows'
    if build_type == 'debug'
        grpc_proj = subproject('grpc_debug')
    else
        grpc_proj = subproject('grpc')
    endif
    grpc_dep = grpc_proj.get_variable('grpc_dep')
    grpc_bin_root = grpc_proj.get_variable('grpc_bin_root')
endif
if not grpc_dep.found()
    error('grpc devkit is required...')
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
# TitanEngine_proj = subproject('TitanEngine')
# TitanEngine_dep = TitanEngine_proj.get_variable('TitanEngine_dep')
# TitanEngine_bin_root = TitanEngine_proj.get_variable('TitanEngine_bin_root')

#
# generate protobuf
#

protoc_gen_grpc_plugin = '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path()
proto_generator = generator(protoc,
                    output: [
                        '@BASENAME@.pb.h',
                        '@BASENAME@.pb.cc',
                        '@BASENAME@.grpc.pb.h',
                        '@BASENAME@.grpc.pb.cc'],
                    arguments: [
                        protoc_gen_grpc_plugin,
                        '--grpc_out=@BUILD_DIR@',
                        '--proto_path=@CURRENT_SOURCE_DIR@/protos',
                        '--cpp_out=@BUILD_DIR@',                                                
                        '@INPUT@'])
proto_generated = proto_generator.process('protos/wxbox.proto')

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
    join_paths(utils_root, 'crack.h'),
    join_paths(utils_root, 'crack.cpp'),
    join_paths(utils_root, 'traits.h'),
    join_paths(utils_root, 'traits.cpp'),
    join_paths(utils_root, 'inject.h'),
    join_paths(utils_root, 'inject.cpp'),
    join_paths(utils_root, 'hook.h'),
    join_paths(utils_root, 'hook.cpp'),
    join_paths(utils_root, 'platform.h'),
    join_paths(utils_root, 'platform.cpp'),
    join_paths(utils_root, 'coredump.h'),
    join_paths(utils_root, 'coredump.cpp'),
    join_paths(utils_root, 'config.hpp'),
    join_paths(utils_root, 'config.cpp')
]

utils_static_lib = static_library('utils',
                        sources: [utils_src],
                        include_directories: [utils_inc, configuration_inc],
                        dependencies: [frida_gum_dep, yaml_cpp_dep],
                        cpp_args: [ignore_warnings])

#
# plugin
#

plugin_inc = [
    source_root
]

plugin_src = [
    join_paths(plugin_root, 'plugin.h'),
    join_paths(plugin_root, 'plugin.cpp'),
    join_paths(plugin_root, 'plugin_virtual_machine.h'),
    join_paths(plugin_root, 'plugin_virtual_machine.cpp'),
    join_paths(plugin_root, 'plugin_command_parser.h'),
    join_paths(plugin_root, 'plugin_command_parser.cpp'),
    join_paths(plugin_root, 'internal/plugin_internal_functions.h'),
    join_paths(plugin_root, 'internal/plugin_internal_functions.cpp'),
    join_paths(plugin_root, 'internal/plugin_internal_modules.h'),
    join_paths(plugin_root, 'internal/plugin_internal_modules.cpp'),
    join_paths(plugin_root, 'internal/module/module_wxbox.h'),
    join_paths(plugin_root, 'internal/module/module_wxbox.cpp'),
    join_paths(plugin_root, 'internal/module/model_wechat_contact.h'),
    join_paths(plugin_root, 'internal/module/model_wechat_contact.cpp'),
    join_paths(plugin_root, 'internal/module/model_event.h'),
    join_paths(plugin_root, 'internal/module/model_event.cpp'),
    join_paths(plugin_root, 'internal/module/model_host_event.h'),
    join_paths(plugin_root, 'internal/module/model_host_event.cpp')
]

plugin_static_lib = static_library('plugin',
                        sources: [plugin_src],
                        include_directories: [utils_inc, plugin_inc, configuration_inc],
                        dependencies: [lua_dep, yaml_cpp_dep],
                        cpp_args: [ignore_warnings])

#
# WxBox
#

subdir('src/wxbox')

#
# WxBot
#

subdir('src/wxbot')

#
# crashdumper
#

subdir('src/crashdumper')

#
# interact
#

subdir('src/interact')

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