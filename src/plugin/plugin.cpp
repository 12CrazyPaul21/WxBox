#include <plugin/plugin.h>

//
// Global Variables
//

static const char* HostEventNames[] = {
    "SendTextMesage",
};

static const char* PluginEventNames[] = {
    "load",
    "prereload",
    "unload",
    "receive_raw_message",
    "receive_text_message",
    "send_text_message",
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
        {"receive_text_message", __plugin_on_receive_text_message_stub},
        {"send_text_message", __plugin_on_send_text_message_stub},
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
    lua_pushstring(L, wb_file::JoinPath(wb_plugin::GetPluginVirtualMachineStorageRoot(), pluginName).c_str());
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
    return std::make_shared<wxbox::plugin::HostEventModel>();
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

    ptr->pCommand    = nullptr;
    ptr->pData       = nullptr;
    ptr->messageType = 0;

    return ptr;
}