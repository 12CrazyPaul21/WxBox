#include <plugin/plugin.h>

//
// plugin event model methods
//

//
// plugin event model object methods
//

static int __plugin_event_get_type(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (int)ptr->type);
    return 1;
}

static int __plugin_event_get_data_ptr(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (lua_Integer)ptr->pData1);
    return 1;
}

static int __plugin_event_get_message_type(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (lua_Integer)ptr->messageType);
    return 1;
}

static int __plugin_event_get_wxid(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->wxid.c_str());
    return 1;
}

static int __plugin_event_get_message(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->message.c_str());
    return 1;
}

static int __plugin_event_get_chatroom_talker_xwid(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->chatroomTalkerWxid.c_str());
    return 1;
}

static int __plugin_event_filter_message(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    if (!ptr->pData1) {
        lua_pushboolean(L, false);
        return 1;
    }

    WECHAT_MESSAGE_FILTER(ptr->pData1);
    lua_pushboolean(L, true);
    return 1;
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::PluginEventModelMethods[] = {
    {NULL, NULL},
};

const struct luaL_Reg wxbox::plugin::internal::PluginEventModelObjectMethods[] = {
    {"type", __plugin_event_get_type},
    {"data_ptr", __plugin_event_get_data_ptr},
    {"message_type", __plugin_event_get_message_type},
    {"wxid", __plugin_event_get_wxid},
    {"message", __plugin_event_get_message},
    {"chatroom_talker_xwid", __plugin_event_get_chatroom_talker_xwid},
    {"filter_message", __plugin_event_filter_message},
    {NULL, NULL},
};

int wxbox::plugin::internal::__luaopen_wxbox_plugin_event_model(lua_State* L)
{
    if (lua_type(L, 1) != LUA_TSTRING) {
        return 0;
    }

    // get type name from stack top
    const char* type = luaL_checkstring(L, lua_gettop(L));
    if (!type || !strlen(type)) {
        return 0;
    }

    // register static methods
    luaL_newlib(L, PluginEventModelMethods);

    // new metatable
    luaL_newmetatable(L, type);

    // config __index table
    luaL_newlib(L, PluginEventModelObjectMethods);
    lua_setfield(L, -2, "__index");

    // balanced
    lua_pop(L, 1);

    return 1;
}

bool wxbox::plugin::internal::IsPluginEventModelMethod(const std::string& methodName)
{
    for (const luaL_Reg* method = PluginEventModelMethods; method->func; method++) {
        if (!methodName.compare(method->name)) {
            return true;
        }
    }
    return false;
}