#include <plugin/plugin.h>

//
// Global Variables
//

static const char* HostEventNames[] = {
    "SendMesage",
    "Log",
    "ClearCommandResultScreen",
    "Logout",
    "ChangeConfig",
    "UnInject",
    "ExitWxBox",
    "ReportHelp",
    "GetPhoneTestCase",
};

static const char* PluginEventNames[] = {
    "load",
    "prereload",
    "unload",
    "receive_raw_message",
    "receive_message",
    "receive_text_message",
    "send_text_message",
    "login_wechat_event",
    "logout_wechat_event",
    "exit_wechat_event",
};

//
// Inner Methods
//

static int __plugin_on_load_stub(lua_State* L)
{
    // true indicates successful initialization, and false indicates failed initialization
    lua_pushboolean(L, 1);
    return 1;
}

static int __plugin_on_prereload_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_unload_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_receive_raw_message_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_receive_message_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_receive_text_message_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_send_text_message_stub(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_login_wechat_event(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_logout_wechat_event(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_exit_wechat_event(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_timer_event(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_on_every_day_timer_event(lua_State* L)
{
    WXBOX_UNREF(L);

    // do nothing
    return 0;
}

static int __plugin_start_timer(lua_State* L)
{
    if (lua_gettop(L) != 3) {
        luaL_argerror(L, lua_gettop(L), "start_timer(id, period)");
        lua_pushboolean(L, false);
        return 1;
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    //
    // get timer args
    //

    lua_Integer timerId = luaL_checkinteger(L, 2);
    luaL_argcheck(L, timerId >= 0, 2, "timer id must equal or greater than 0");

    lua_Integer period = luaL_checkinteger(L, 3);
    luaL_argcheck(L, period >= 10, 3, "timer period must equal or greater than 10 ms");

    //
    // get plugin name
    //

    if (!lua_getfield(L, -3, "plugin_name")) {
        luaL_argerror(L, lua_gettop(L), "get plugin name failed");
        lua_pushboolean(L, false);
        return 1;
    }

    const char* pluginName = luaL_checkstring(L, -1);
    luaL_argcheck(L, pluginName != nullptr, 1, "invalid plugin name");

    //
    // start plugin timer
    //

    lua_pushboolean(L, wb_plugin::StartPluginTimer(pluginName, (int)timerId, (int)period));
    return 1;
}

static int __plugin_start_every_day_timer(lua_State* L)
{
    if (lua_gettop(L) != 5) {
        luaL_argerror(L, lua_gettop(L), "start_every_day_timer(id, hour, minute, second)");
        lua_pushboolean(L, false);
        return 1;
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    //
    // get timer args
    //

    lua_Integer timerId = luaL_checkinteger(L, 2);
    luaL_argcheck(L, timerId >= 0, 2, "timer id must equal or greater than 0");

    lua_Integer hour = luaL_checkinteger(L, 3);
    luaL_argcheck(L, hour >= 0 && hour < 24, 3, "hour[0~23]");

    lua_Integer minute = luaL_checkinteger(L, 4);
    luaL_argcheck(L, minute >= 0 && minute < 60, 4, "minute[0~59]");

    lua_Integer second = luaL_checkinteger(L, 5);
    luaL_argcheck(L, minute >= 0 && minute < 60, 5, "second[0~59]");

    //
    // get plugin name
    //

    if (!lua_getfield(L, -5, "plugin_name")) {
        luaL_argerror(L, lua_gettop(L), "get plugin name failed");
        lua_pushboolean(L, false);
        return 1;
    }

    const char* pluginName = luaL_checkstring(L, -1);
    luaL_argcheck(L, pluginName != nullptr, 1, "invalid plugin name");

    //
    // start plugin timer
    //

    lua_pushboolean(L, wb_plugin::StartPluginTimer(pluginName, (int)timerId, wb_timer::EveryDayPeriodDesc((uint8_t)hour, (uint8_t)minute, (uint8_t)second)));
    return 1;
}

static int __plugin_kill_timer(lua_State* L)
{
    if (lua_gettop(L) != 2) {
        luaL_argerror(L, lua_gettop(L), "kill_timer(id)");
        return 0;
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    //
    // get timer args
    //

    lua_Integer timerId = luaL_checkinteger(L, 2);
    luaL_argcheck(L, timerId >= 0, 2, "timer id must equal or greater than 0");

    //
    // get plugin name
    //

    if (!lua_getfield(L, -2, "plugin_name")) {
        luaL_argerror(L, lua_gettop(L), "get plugin name failed");
        return 0;
    }

    const char* pluginName = luaL_checkstring(L, -1);
    luaL_argcheck(L, pluginName != nullptr, 1, "invalid plugin name");

    //
    // stop plugin timer
    //

    wb_plugin::StopPluginTimer(pluginName, (int)timerId, true);
    return 0;
}

static int __plugin_kill_every_day_timer(lua_State* L)
{
    if (lua_gettop(L) != 2) {
        luaL_argerror(L, lua_gettop(L), "kill_every_day_timer(id)");
        return 0;
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    //
    // get timer args
    //

    lua_Integer timerId = luaL_checkinteger(L, 2);
    luaL_argcheck(L, timerId >= 0, 2, "timer id must equal or greater than 0");

    //
    // get plugin name
    //

    if (!lua_getfield(L, -2, "plugin_name")) {
        luaL_argerror(L, lua_gettop(L), "get plugin name failed");
        return 0;
    }

    const char* pluginName = luaL_checkstring(L, -1);
    luaL_argcheck(L, pluginName != nullptr, 1, "invalid plugin name");

    //
    // stop plugin timer
    //

    wb_plugin::StopPluginTimer(pluginName, (int)timerId, false);
    return 0;
}

/**
 *
 * global function "declare_module":
 *		<table> declare_plugin(string)
 *
 * parameter:
 *		#1<string>: plugin name
 *
 * return:
 *		#1<table>:  empty module
 *
 * usage<in *.lua script>:
 *		example = declare_plugin("example")
 *
 */
int wxbox::plugin::__declare_plugin(lua_State* L)
{
    static const struct luaL_Reg MODULE_HANDLER_STUB[] = {
        {"load", __plugin_on_load_stub},
        {"prereload", __plugin_on_prereload_stub},
        {"unload", __plugin_on_unload_stub},
        {"receive_raw_message", __plugin_on_receive_raw_message_stub},
        {"receive_message", __plugin_on_receive_message_stub},
        {"receive_text_message", __plugin_on_receive_text_message_stub},
        {"send_text_message", __plugin_on_send_text_message_stub},
        {"login_wechat_event", __plugin_on_login_wechat_event},
        {"logout_wechat_event", __plugin_on_logout_wechat_event},
        {"exit_wechat_event", __plugin_on_exit_wechat_event},
        {"timer_event", __plugin_on_timer_event},
        {"every_day_timer_event", __plugin_on_every_day_timer_event},
        {"start_timer", __plugin_start_timer},
        {"start_every_day_timer", __plugin_start_every_day_timer},
        {"kill_timer", __plugin_kill_timer},
        {"kill_every_day_timer", __plugin_kill_every_day_timer},
        {NULL, NULL},
    };

    if (lua_gettop(L) < 1) {
        return 0;
    }

    // check parameter
    if (lua_type(L, 1) != LUA_TSTRING) {
        return 0;
    }

    // get plugin name
    const char* pluginName = lua_tostring(L, 1);
    if (!pluginName || !pluginName[0]) {
        return 0;
    }

    //
    // build a table for new plugin
    //

    lua_newtable(L);

    // record plugin name
    lua_pushstring(L, pluginName);
    lua_setfield(L, -2, "plugin_name");

    // record plugin storage path
    lua_pushstring(L, wb_file::JoinPath(wb_string::NativeToUtf8String(wb_plugin::GetPluginVirtualMachineStorageRoot()), pluginName).c_str());
    lua_setfield(L, -2, "storage_path");

    // register event handler stubs
    for (const luaL_Reg* stub = MODULE_HANDLER_STUB; stub->func; stub++) {
        lua_pushcfunction(L, stub->func);
        lua_setfield(L, -2, stub->name);
    }

    return 1;
}

bool wxbox::plugin::TriggerPluginOnLoad(PPluginVirtualMachine vm, const std::string& pluginName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginName.empty()) {
        return false;
    }

    // check whether plugin is a table object
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TTABLE) {
        lua_pop(vm->state, 1);
        return false;
    }

    // check whether function is exist
    if (lua_getfield(vm->state, -1, "load") != LUA_TFUNCTION) {
        lua_pop(vm->state, 2);
        return false;
    }

    // trigger event with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, 0, 1, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 2);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    // check result
    if (lua_type(vm->state, -1) != LUA_TBOOLEAN) {
        lua_pop(vm->state, 2);
        return false;
    }

    bool retval = lua_toboolean(vm->state, -1);
    lua_pop(vm->state, 2);
    return retval;
}

bool wxbox::plugin::TriggerPluginOnUnLoad(PPluginVirtualMachine vm, const std::string& pluginName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginName.empty()) {
        return false;
    }

    // check whether plugin is a table object
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TTABLE) {
        lua_pop(vm->state, 1);
        return false;
    }

    // check whether function is exist
    if (lua_getfield(vm->state, -1, "unload") != LUA_TFUNCTION) {
        lua_pop(vm->state, 2);
        return false;
    }

    // trigger event with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, 0, 0, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 2);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    lua_pop(vm->state, 1);
    return true;
}

bool wxbox::plugin::TriggerPluginOnPreReLoad(PPluginVirtualMachine vm, const std::string& pluginName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginName.empty()) {
        return false;
    }

    // check whether plugin is a table object
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TTABLE) {
        lua_pop(vm->state, 1);
        return false;
    }

    // check whether function is exist
    if (lua_getfield(vm->state, -1, "prereload") != LUA_TFUNCTION) {
        lua_pop(vm->state, 2);
        return false;
    }

    // trigger event with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, 0, 0, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 2);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    lua_pop(vm->state, 1);
    return true;
}

bool wxbox::plugin::TriggerPluginEvent(PPluginVirtualMachine vm, const std::string& pluginName, wxbox::plugin::PluginEventModelPtr pluginEvent)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginName.empty() || !pluginEvent) {
        return false;
    }

    auto eventType = PluginEventTypeToString(pluginEvent->type);
    if (eventType.empty()) {
        return false;
    }

    // check whether plugin is a table object
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TTABLE) {
        lua_pop(vm->state, 1);
        return false;
    }

    // check whether function is exist
    if (lua_getfield(vm->state, -1, eventType.c_str()) != LUA_TFUNCTION) {
        lua_pop(vm->state, 2);
        return false;
    }

    // push event
    PushUserDataPtrToStack<wb_plugin::PluginEventModel, EVENT_MODEL_NAME>(vm->state, pluginEvent.get());

    // trigger event with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, 1, 0, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 2);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    lua_pop(vm->state, 1);
    return true;
}

bool wxbox::plugin::TriggerPluginTimerEvent(_PluginVirtualMachine* vm, const std::string& pluginName, int id, bool isPeriodTimer)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginName.empty()) {
        return false;
    }

    // check whether plugin is a table object
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TTABLE) {
        lua_pop(vm->state, 1);
        return false;
    }

    // check whether timer event method is exist
    if (lua_getfield(vm->state, -1, isPeriodTimer ? "timer_event" : "every_day_timer_event") != LUA_TFUNCTION) {
        lua_pop(vm->state, 2);
        return false;
    }

    // push timer event parameters
    lua_pushinteger(vm->state, id);

    // trigger event with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, 1, 0, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 2);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    lua_pop(vm->state, 1);
    return true;
}

bool wxbox::plugin::IsPluginEventName(const std::string& methodName)
{
    for (auto event : PluginEventNames) {
        if (!methodName.compare(event)) {
            return true;
        }
    }

    return false;
}

std::string wxbox::plugin::HostEventTypeToString(HostEventType type)
{
    if ((int)type >= sizeof(HostEventNames) / sizeof(char*)) {
        return "";
    }
    return HostEventNames[(int)type];
}

wxbox::plugin::HostEventModelPtr wxbox::plugin::BuildHostEventModel()
{
    auto ptr = std::make_shared<wxbox::plugin::HostEventModel>();
    if (!ptr) {
        return nullptr;
    }

    ptr->sendMessageArgs = nullptr;
    ptr->log             = nullptr;
    ptr->changeConfig    = nullptr;

    return ptr;
}

wxbox::plugin::HostEventModelPtr wxbox::plugin::CopyHostEventModel(HostEventModel* model)
{
    if (!model) {
        return std::make_shared<wxbox::plugin::HostEventModel>();
    }
    return std::make_shared<wxbox::plugin::HostEventModel>(*model);
}

std::string wxbox::plugin::PluginEventTypeToString(PluginEventType type)
{
    if ((int)type >= sizeof(PluginEventNames) / sizeof(char*)) {
        return "";
    }
    return PluginEventNames[(int)type];
}

wxbox::plugin::PluginEventModelPtr wxbox::plugin::BuildPluginEventModel()
{
    auto ptr = std::make_shared<wxbox::plugin::PluginEventModel>();
    if (!ptr) {
        return nullptr;
    }

    ptr->pData1      = nullptr;
    ptr->pData2      = nullptr;
    ptr->messageType = 0;

    return ptr;
}