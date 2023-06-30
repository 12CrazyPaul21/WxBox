#ifndef __WXBOX_DYNAMIC_PLUGIN_ASYNC_REQUEST_H
#define __WXBOX_DYNAMIC_PLUGIN_ASYNC_REQUEST_H

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
#include <string>

typedef struct _RequestContext
{
	std::string url;
	std::string context;
} RequestContext, * PRequestContext;

WXBOX_DYNAMIC_PLUGIN_PUBLIC_API int LUA_API luaopen_async_request(lua_State* L);


#endif  // #ifndef __WXBOX_DYNAMIC_PLUGIN_ASYNC_REQUEST_H