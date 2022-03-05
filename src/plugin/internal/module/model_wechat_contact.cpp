#include <plugin/plugin.h>

//
// wechat contact model methods
//

static int __wxcontact_create(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::PushUserDataToStack<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    return ptr ? 1 : 0;
}

//
// wechat contact model object methods
//

static int __wxcontact_gc(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    if (ptr) {
        delete ptr;
    }
    return 0;
}

static int __wxcontact_get_wxid(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->wxid.c_str());
    return 1;
}

static int __wxcontact_set_wxid(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* wxid = luaL_checkstring(L, 2);
    luaL_argcheck(L, wxid != nullptr, 2, "parameter must be a string");

    ptr->wxid = const_cast<char*>(wxid);
    return 0;
}

static int __wxcontact_get_username(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->username.c_str());
    return 1;
}

static int __wxcontact_set_username(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* username = luaL_checkstring(L, 2);
    luaL_argcheck(L, username != nullptr, 2, "parameter must be a string");

    ptr->username = const_cast<char*>(username);
    return 0;
}

static int __wxcontact_get_nickname(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->nickname.c_str());
    return 1;
}

static int __wxcontact_set_nickname(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* nickname = luaL_checkstring(L, 2);
    luaL_argcheck(L, nickname != nullptr, 2, "parameter must be a string");

    ptr->nickname = const_cast<char*>(nickname);
    return 0;
}

static int __wxcontact_get_remark(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->remark.c_str());
    return 1;
}

static int __wxcontact_set_remark(lua_State* L)
{
    wb_plugin_internal::WxContactModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin_internal::WxContactModel, WXCONTACT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* remark = luaL_checkstring(L, 2);
    luaL_argcheck(L, remark != nullptr, 2, "parameter must be a string");

    ptr->remark = const_cast<char*>(remark);
    return 0;
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::WxContactModelMethods[] = {
    {"create", __wxcontact_create},
    {NULL, NULL},
};

const struct luaL_Reg wxbox::plugin::internal::WxContactModelObjectMethods[] = {
    {"wxid", __wxcontact_get_wxid},
    {"set_wxid", __wxcontact_set_wxid},
    {"username", __wxcontact_get_username},
    {"set_username", __wxcontact_set_username},
    {"nickname", __wxcontact_get_nickname},
    {"set_nickname", __wxcontact_set_nickname},
    {"remark", __wxcontact_get_remark},
    {"set_remark", __wxcontact_set_remark},
    {NULL, NULL},
};

int wxbox::plugin::internal::__luaopen_wx_contact_model(lua_State* L)
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
    luaL_newlib(L, WxContactModelMethods);

    // new metatable
    luaL_newmetatable(L, type);

    // config __index table
    luaL_newlib(L, WxContactModelObjectMethods);
    lua_setfield(L, -2, "__index");

    // register __gc function
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, __wxcontact_gc);
    lua_settable(L, -3);

    // balanced
    lua_pop(L, 1);

    return 1;
}

bool wxbox::plugin::internal::IsWxContactModelMethod(const std::string& methodName)
{
    for (const luaL_Reg* method = WxContactModelMethods; method->func; method++) {
        if (!methodName.compare(method->name)) {
            return true;
        }
    }
    return false;
}