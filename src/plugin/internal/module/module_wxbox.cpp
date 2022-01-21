#include <plugin/plugin.h>

//
// wxbox module methods
//

static int __wxbox_version(lua_State* L)
{
    std::stringstream ss;
    ss << "wxbot-v" << WXBOT_MODULE_VERSION;

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

static int __wxbox_package_storage_path(lua_State* L)
{
    const char* moduleName = luaL_checkstring(L, 1);
    luaL_argcheck(L, moduleName != nullptr, 1, "module name is required");

    lua_pushstring(L, wb_file::JoinPath(wb_plugin::GetPluginVirtualMachineStorageRoot(), moduleName).c_str());
    return 1;
}

static int __wxbox_dispatch_host_event(lua_State* L)
{
    auto ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    auto duplicate = wb_plugin::CopyHostEventModel(ptr);
    if (!duplicate) {
        return 0;
    }

    wb_plugin::DispatchPluginToHostEvent(duplicate);
    return 0;
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::WxBoxModuleMethods[] = {
    {"version", __wxbox_version},
    {"package_storage_path", __wxbox_package_storage_path},
    {"dispatch_host_event", __wxbox_dispatch_host_event},
    {NULL, NULL},
};

int wxbox::plugin::internal::__luaopen_wxbox_module(lua_State* L)
{
    luaL_newlib(L, WxBoxModuleMethods);
    return 1;
}

bool wxbox::plugin::internal::IsWxBoxModuleMethod(const std::string& methodName)
{
    for (const luaL_Reg* method = WxBoxModuleMethods; method->func; method++) {
        if (!methodName.compare(method->name)) {
            return true;
        }
    }
    return false;
}