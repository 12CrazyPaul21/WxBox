project('AsyncRequest', ['c', 'cpp'],
    version : '0.1.0', default_options : ['warning_level=3', 'cpp_std=c++17'])

wxbox_plugin_name = 'async_request'
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
# curl dependency
#

curl_inc = include_directories(join_paths(proj_root, 'thirdparty/curl/include'))
curl_lib = meson.get_compiler('cpp').find_library('curl.dll', dirs: [join_paths(proj_root, 'thirdparty/curl/lib')])
curl_dep = declare_dependency(
    include_directories: curl_inc,
    dependencies: curl_lib
)

#
# generate library
#

shlib = shared_library(wxbox_plugin_name, ['async_request.hpp', 'async_request.cpp'],
    install: true,
    install_dir: join_paths(proj_root, '..'),
    cpp_args : lib_args,
    link_args: ['/DELAYLOAD:libcurl.dll', '-lDelayimp'],
    gnu_symbol_visibility : 'hidden',
    dependencies: [lua_dep, curl_dep]
)