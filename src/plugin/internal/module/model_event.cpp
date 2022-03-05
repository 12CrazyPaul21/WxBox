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

static int __plugin_event_get_chatroom_talker_wxid(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushstring(L, ptr->chatroomTalkerWxid.c_str());
    return 1;
}

static int __plugin_event_get_data1(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (lua_Integer)ptr->pData1);
    return 1;
}

static int __plugin_event_get_data2(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    lua_pushinteger(L, (lua_Integer)ptr->pData2);
    return 1;
}

// only for receive raw message
static int __plugin_event_filter_message(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    if (!ptr->pData1) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (ptr->type != wb_plugin::PluginEventType::ReceiveRawMessage) {
        lua_pushboolean(L, false);
        return 1;
    }

    WECHAT_MESSAGE_FILTER(ptr->pData1);
    lua_pushboolean(L, true);
    return 1;
}

// only for receive raw message and send text message
static int __plugin_event_substitute_wxid(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* newWxid = lua_tostring(L, 2);
    luaL_argcheck(L, newWxid != nullptr, 2, "invalid wxid");

    if (!ptr->pData1) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (ptr->type != wb_plugin::PluginEventType::ReceiveRawMessage && ptr->type != wb_plugin::PluginEventType::SendTextMessage) {
        lua_pushboolean(L, false);
        return 1;
    }

    wb_wx::PWeChatWString pWxidInfo = TO_WECHAT_WSTRING(ptr->pData1);
    if (ptr->type == wb_plugin::PluginEventType::ReceiveRawMessage) {
        auto pMessage = TO_WECHAT_MESSAGE(ptr->pData1);
        if (!pMessage->talker_wxid) {
            lua_pushboolean(L, false);
            return 1;
        }
        pWxidInfo = TO_WECHAT_WSTRING(&pMessage->talker_wxid);
    }

    wb_crack::SubstituteWeChatWString(pWxidInfo, wb_string::ToUtf8WString(newWxid));

    lua_pushboolean(L, true);
    return 1;
}

// only for receive raw message and send text message
static int __plugin_event_substitute_message(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    wb_wx::PWeChatWString pMessageInfo = nullptr;

    if (ptr->type == wb_plugin::PluginEventType::SendTextMessage) {
        if (!ptr->pData2) {
            lua_pushboolean(L, false);
            return 1;
        }
        pMessageInfo = TO_WECHAT_WSTRING(ptr->pData2);
    }
    else if (ptr->type == wb_plugin::PluginEventType::ReceiveRawMessage) {
        if (!ptr->pData1) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto pWeChatMessage = TO_WECHAT_MESSAGE(ptr->pData1);
        if (!pWeChatMessage->message) {
            lua_pushboolean(L, false);
            return 1;
        }
        pMessageInfo = TO_WECHAT_WSTRING(&pWeChatMessage->message);
    }
    else {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* newMessage = lua_tostring(L, 2);
    luaL_argcheck(L, newMessage != nullptr, 2, "invalid message");

    wb_crack::SubstituteWeChatWString(pMessageInfo, wb_string::ToUtf8WString(newMessage));

    lua_pushboolean(L, true);
    return 1;
}

// only for receive raw message
static int __plugin_event_substitute_chatroom_talker_wxid(lua_State* L)
{
    wb_plugin::PluginEventModel* ptr = wb_plugin::FetchUserDataPointer<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    const char* newChatroomTalkerWxid = lua_tostring(L, 2);
    luaL_argcheck(L, newChatroomTalkerWxid != nullptr, 2, "invalid wxid");

    if (!ptr->pData1) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (ptr->type != wb_plugin::PluginEventType::ReceiveRawMessage) {
        lua_pushboolean(L, false);
        return 1;
    }

    auto pMessage = TO_WECHAT_MESSAGE(ptr->pData1);
    if (!pMessage->chatroom_talker_wxid) {
        lua_pushboolean(L, false);
        return 1;
    }

    wb_crack::SubstituteWeChatWString(TO_WECHAT_WSTRING(&pMessage->chatroom_talker_wxid), wb_string::ToUtf8WString(newChatroomTalkerWxid));

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
    {"message_type", __plugin_event_get_message_type},
    {"wxid", __plugin_event_get_wxid},
    {"message", __plugin_event_get_message},
    {"chatroom_talker_wxid", __plugin_event_get_chatroom_talker_wxid},
    {"pdata1", __plugin_event_get_data1},
    {"pdata2", __plugin_event_get_data2},
    {"filter_message", __plugin_event_filter_message},
    {"substitute_wxid", __plugin_event_substitute_wxid},
    {"substitute_message", __plugin_event_substitute_message},
    {"substitute_chatroom_talker_wxid", __plugin_event_substitute_chatroom_talker_wxid},
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