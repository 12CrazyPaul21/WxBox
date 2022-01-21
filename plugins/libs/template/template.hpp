#ifndef __WXBOX_DYNAMIC_PLUGIN_TEMPLATE_H
#define __WXBOX_DYNAMIC_PLUGIN_TEMPLATE_H

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_WXBOX_DYNAMIC_PLUGIN
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC __declspec(dllexport)
#else
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC __declspec(dllimport)
#endif
#else
#ifdef BUILDING_WXBOX_DYNAMIC_PLUGIN
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC __attribute__((visibility("default")))
#else
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC
#endif
#endif

#ifdef __cplusplus
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC_API extern "C" WXBOX_DYNAMIC_PLUGIN_PUBLIC
#else
#define WXBOX_DYNAMIC_PLUGIN_PUBLIC_API WXBOX_DYNAMIC_PLUGIN_PUBLIC
#endif

#include <lua.hpp>

WXBOX_DYNAMIC_PLUGIN_PUBLIC_API int LUA_API luaopen_template(lua_State* L);

#endif  // #ifndef __WXBOX_DYNAMIC_PLUGIN_TEMPLATE_H