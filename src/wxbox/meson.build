wxbox_internal = join_paths(wxbox_root, 'internal')
wxbox_ui = join_paths(wxbox_root, 'ui')

moc_headers = files([join_paths(wxbox_root, 'mainwindow.h'),
                        join_paths(wxbox_root, 'about.h'),
                        join_paths(wxbox_root, 'wxbox_server.hpp'),
                        join_paths(wxbox_root, 'wxbox_controller.h'),
                        join_paths(wxbox_root, 'download_dialog.h'),
                        join_paths(wxbox_internal, 'downloader.hpp')
])
# moc_sources = files([join_paths(wxbox_root, 'mainwindow.cpp'), join_paths(wxbox_root, 'about.cpp')])
ui_files = files([join_paths(wxbox_ui, 'mainwindow.ui'), join_paths(wxbox_ui, 'about.ui'), join_paths(wxbox_ui, 'download.ui')])
qt_resources = files([join_paths(assets_root, 'wxbox.qrc')])

# process about qt
qt_processed = qt5.preprocess(
    moc_headers: moc_headers,
    # moc_sources: moc_sources,
    ui_files: ui_files,
    qresources: [qt_resources, xstyle_resources]
)

wxbox_inc = [
    utils_inc,
    xstyle_inc,
    configuration_inc
]

wxbox_src = files([
    join_paths(wxbox_root, 'main.cpp'),
    join_paths(wxbox_root, 'mainwindow.cpp'),
    join_paths(wxbox_root, 'wxbox_controller_error.h'),
    join_paths(wxbox_root, 'wxbox_controller.cpp'),
    join_paths(wxbox_root, 'about.cpp'),
    join_paths(wxbox_root, 'download_dialog.cpp'),
    join_paths(wxbox_root, 'wxbox_server.cpp'),
    join_paths(wxbox_root, 'app_log.hpp'),
    join_paths(wxbox_root, 'app_config.hpp'),
    join_paths(wxbox_internal, 'threadpool.hpp')
])

# resources
if host_os == 'windows'
    windows = import('windows')
    wxbox_src += windows.compile_resources(join_paths(assets_root, 'wxbox.rc'), include_directories: configuration_inc)
elif host_os == 'darwin'
    # apple
endif

# add wxbox executable
executable('wxbox',
    sources: [wxbox_src, qt_processed, proto_generated],
    include_directories: wxbox_inc,
    gui_app: true,
    install : true,
    install_dir: project_install_root,
    dependencies: [qt5_dep, spdlog_dep, grpc_dep, yaml_cpp_dep],
    cpp_args: [ignore_warnings, config_macro_flags, xstyle_flags],
    link_with: [utils_static_lib, xstyle_static_lib]
)