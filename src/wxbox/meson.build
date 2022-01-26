moc_headers = [join_paths(wxbox_root, 'mainwindow.h'), join_paths(wxbox_root, 'about.h'), join_paths(wxbox_root, 'wxbox_server.hpp')]
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

wxbox_internal = join_paths(wxbox_root, 'internal')

wxbox_inc = [
    utils_inc,
    configuration_inc
]

wxbox_src = [
    join_paths(wxbox_root, 'main.cpp'),
    join_paths(wxbox_root, 'mainwindow.cpp'),
    join_paths(wxbox_root, 'about.cpp'),
    join_paths(wxbox_root, 'wxbox_server.cpp'),
    join_paths(wxbox_root, 'app_log.hpp'),
    join_paths(wxbox_root, 'app_config.hpp'),
    join_paths(wxbox_internal, 'threadpool.hpp')
]

# resources
if host_os == 'windows'
    windows = import('windows')
    wxbox_src += windows.compile_resources(join_paths(assets_root, 'wxbox.rc'))
elif host_os == 'darwin'
    # apple
endif

# add wxbox executable
executable('wxbox',
    sources: [wxbox_src, qt_processed, proto_generated],
    include_directories: wxbox_inc,
    gui_app: true,
    install : true,
    dependencies: [qt5_dep, spdlog_dep, grpc_dep, yaml_cpp_dep],
    cpp_args: [ignore_warnings, config_macro_flags],
    link_with: utils_static_lib
)