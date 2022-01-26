crashdumper_moc_headers = [join_paths(crashdumper_root, 'crashdumper_dialog.h')]
crashdumper_moc_sources = [join_paths(crashdumper_root, 'crashdumper_dialog.cpp')]
crashdumper_ui_files = [join_paths(crashdumper_root, 'crashdumper_dialog.ui')]
crashdumper_qt_resources = [join_paths(crashdumper_assets_root, 'crashdumper.qrc')]

# process about qt
crashdumper_qt_processed = qt5.preprocess(
    moc_headers: crashdumper_moc_headers,
    # moc_sources: crashdumper_moc_sources,
    ui_files: crashdumper_ui_files,
    qresources: crashdumper_qt_resources
)

crashdumper_inc = [
    utils_inc,
    configuration_inc
]

crashdumper_src = [
    join_paths(crashdumper_root, 'crashdumper.h'),
    join_paths(crashdumper_root, 'crashdumper.cpp'),
    join_paths(crashdumper_root, 'crashdumper_dialog.cpp')
]

# resources
if host_os == 'windows'
    windows = import('windows')
    crashdumper_src += windows.compile_resources(join_paths(crashdumper_assets_root, 'crashdumper.rc'))
elif host_os == 'darwin'
    # apple
endif

# add crashdumper executable
executable('crashdumper',
    sources: [crashdumper_src, crashdumper_qt_processed],
    include_directories: crashdumper_inc,
    gui_app: true,
    dependencies: [qt5_dep, yaml_cpp_dep],
    cpp_args: [ignore_warnings],
    link_with: [utils_static_lib]
)