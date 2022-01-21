#include <plugin/plugin.h>

extern int wxbox::plugin::__declare_plugin(lua_State* L);

//
// wxbox plugin virtual machine internal functions
//

// ...

//
// export plugin virtual machine internal functions
//

const struct luaL_Reg wxbox::plugin::internal::PluginVirtualMachineInternalFunctions[] = {
    {"declare_plugin", wxbox::plugin::__declare_plugin},
    {NULL, NULL}};

//
// export function
//

bool wxbox::plugin::internal::IsPluginVirtualMachineInternalFuncion(const std::string& funcName)
{
    for (const luaL_Reg* func = PluginVirtualMachineInternalFunctions; func->func; func++) {
        if (!funcName.compare(func->name)) {
            return true;
        }
    }
    return false;
}