#include "template.hpp"

#include <sstream>

static int __template_hello(lua_State* L)
{
    std::stringstream ss;
    ss << "hello im [" << WXBOX_PLUGIN_NAME << "] dynamic module";

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

const struct luaL_Reg TemplateModuleMethods[] = {
    {"hello", __template_hello},
    {NULL, NULL},
};

int LUA_API luaopen_template(lua_State* L)
{
    luaL_newlib(L, TemplateModuleMethods);
    return 1;
}