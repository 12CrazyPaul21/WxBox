project('WxBoxDynamicPluginTemplate', ['c', 'cpp'],
    version : '0.1.0', default_options : ['warning_level=3', 'cpp_std=c++17'])

wxbox_plugin_name = 'template'
wxbox_plugin_name_macro = '-DWXBOX_PLUGIN_NAME="' + wxbox_plugin_name + '"'

proj_root = meson.current_source_dir()

#
# compiler flag
#

lib_args = ['-DBUILDING_WXBOX_DYNAMIC_PLUGIN', wxbox_plugin_name_macro]

#
# lua dependency
#

lua_proj = subproject('lua')
lua_dep = lua_proj.get_variable('lua_dep')

#
# generate library
#

shlib = shared_library(wxbox_plugin_name, ['template.hpp', 'template.cpp'],
    install: true,
    install_dir: join_paths(proj_root, '..'),
    cpp_args : lib_args,
    gnu_symbol_visibility : 'hidden',
    dependencies: [lua_dep]
)