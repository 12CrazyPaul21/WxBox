#include <plugin/plugin.h>

//
// host event model methods
//

static int __host_event_create(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::PushUserDataToStack<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    return ptr ? 1 : 0;
}

//
// host event model object methods
//

static int __host_event_gc(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    if (ptr) {
        delete ptr;
    }
    return 0;
}

static int __host_event_get_type(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (int)ptr->type);
    return 1;
}

static int __host_event_set_type(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");
    luaL_argcheck(L, lua_isinteger(L, 2), 2, "parameter must be a integer");

    ptr->type = (wb_plugin::HostEventType)lua_tointeger(L, 2);
    return 0;
}

static int __host_event_get_wxid(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->wxid.c_str());
    return 1;
}

static int __host_event_set_wxid(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* wxid = luaL_checkstring(L, 2);
    luaL_argcheck(L, wxid != nullptr, 2, "parameter must be a string");

    ptr->wxid = const_cast<char*>(wxid);
    return 0;
}

static int __host_event_get_text_message(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->textMessage.c_str());
    return 1;
}

static int __host_event_set_text_message(lua_State* L)
{
    wb_plugin::HostEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* textMessage = luaL_checkstring(L, 2);
    luaL_argcheck(L, textMessage != nullptr, 2, "parameter must be a string");

    ptr->textMessage = const_cast<char*>(textMessage);
    return 0;
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::HostEventModelMethods[] = {
    {"create", __host_event_create},
    {NULL, NULL},
};

const struct luaL_Reg wxbox::plugin::internal::HostEventModelObjectMethods[] = {
    {"type", __host_event_get_type},
    {"set_type", __host_event_set_type},
    {"wxid", __host_event_get_wxid},
    {"set_wxid", __host_event_set_wxid},
    {"text_message", __host_event_get_text_message},
    {"set_text_message", __host_event_set_text_message},
    {NULL, NULL},
};

int wxbox::plugin::internal::__luaopen_wxbox_host_event_model(lua_State* L)
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
    luaL_newlib(L, HostEventModelMethods);

    // new metatable
    luaL_newmetatable(L, type);

    // config __index table
    luaL_newlib(L, HostEventModelObjectMethods);
    lua_setfield(L, -2, "__index");

    // register __gc function
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, __host_event_gc);
    lua_settable(L, -3);

    // register event type
    for (int i = 0; i < (int)wb_plugin::HostEventType::_TotalHostEventType; i++) {
        auto eventTypeName = wb_plugin::HostEventTypeToString((wb_plugin::HostEventType)i);
        if (eventTypeName.empty()) {
            continue;
        }
        lua_pushinteger(L, i);
        lua_setfield(L, -3, eventTypeName.c_str());
    }

    // balanced
    lua_pop(L, 1);
    return 1;
}

bool wxbox::plugin::internal::IsHostEventModelMethod(const std::string& methodName)
{
    for (const luaL_Reg* method = HostEventModelMethods; method->func; method++) {
        if (!methodName.compare(method->name)) {
            return true;
        }
    }
    return false;
}