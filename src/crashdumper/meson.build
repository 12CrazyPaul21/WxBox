crashdumper_moc_headers = files([join_paths(crashdumper_root, 'crashdumper_dialog.h')])
crashdumper_moc_sources = files([join_paths(crashdumper_root, 'crashdumper_dialog.cpp')])
crashdumper_ui_files = files([join_paths(crashdumper_root, 'crashdumper_dialog.ui')])
crashdumper_qt_resources = files([join_paths(crashdumper_assets_root, 'crashdumper.qrc')])

# process about qt
crashdumper_qt_processed = qt5.preprocess(
    moc_headers: crashdumper_moc_headers,
    # moc_sources: crashdumper_moc_sources,
    ui_files: crashdumper_ui_files,
    qresources: [crashdumper_qt_resources, xstyle_resources]
)

crashdumper_inc = [
    utils_inc,
    xstyle_inc,
    configuration_inc
]

crashdumper_src = files([
    join_paths(crashdumper_root, 'crashdumper.h'),
    join_paths(crashdumper_root, 'crashdumper.cpp'),
    join_paths(crashdumper_root, 'crashdumper_dialog.cpp')
])

# resources
if host_os == 'windows'
    windows = import('windows')
    crashdumper_src += windows.compile_resources(join_paths(crashdumper_assets_root, 'crashdumper.rc'), include_directories: configuration_inc)
elif host_os == 'darwin'
    # apple
endif

# add crashdumper executable
executable('crashdumper',
    sources: [crashdumper_src, crashdumper_qt_processed],
    include_directories: crashdumper_inc,
    gui_app: true,
    install: true,
    install_dir: project_install_root,
    dependencies: [qt5_dep, yaml_cpp_dep],
    cpp_args: [ignore_warnings, xstyle_flags],
    link_with: [utils_static_lib, xstyle_static_lib]
)