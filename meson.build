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

# frida-gum(only for WxBot)
frida_gum_dep = dependency('frida-gum', required: false)
if not frida_gum_dep.found()
    frida_gum_proj = subproject('frida-gum')
    frida_gum_dep = frida_gum_proj.get_variable('frida_gum_dep')
endif

# lua(only for WxBot)
lua_proj = subproject('lua')
lua_dep = lua_proj.get_variable('lua_dep')

#
# WxBox
#

moc_headers = [join_paths(wxbox_root, 'mainwindow.h'), join_paths(wxbox_root, 'about.h')]
moc_sources = [join_paths(wxbox_root, 'mainwindow.cpp'), join_paths(wxbox_root, 'about.cpp')]
ui_files = [join_paths(wxbox_root, 'mainwindow.ui'), join_paths(wxbox_root, 'about.ui')]
qt_resources = [join_paths(assets_root, 'wxbox.qrc')]

# process about qt
qt_processed = qt5.preprocess(
    moc_headers: moc_headers,
    # moc_sources: moc_sources,
    ui_files: ui_files,
    qresources: qt_resources
)

utils_inc = [
    utils_root
]

utils_src = [
    join_paths(utils_root, 'process.h'),
    join_paths(utils_root, 'process.cpp'),
    join_paths(utils_root, 'wx.h'),
    join_paths(utils_root, 'wx.cpp')
]

wxbox_src = [
    join_paths(wxbox_root, 'main.cpp'),
    join_paths(wxbox_root, 'mainwindow.cpp'),
    join_paths(wxbox_root, 'about.cpp'),
    utils_src
]

# resources
if host_os == 'windows'
    windows = import('windows')
    wxbox_src += windows.compile_resources(join_paths(assets_root, 'wxbox.rc'))
elif host_os == 'darwin'
    # 暂时不管苹果
endif

# add wxbox executable
exe = executable('wxbox', sources: [wxbox_src, qt_processed], include_directories: utils_inc,
    install : true, dependencies: [qt5_dep, spdlog_dep, grpc_dep], gui_app: true)

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