#ifndef __WXBOX_PLUGIN_INTERNAL_FUNCTIONS_H
#define __WXBOX_PLUGIN_INTERNAL_FUNCTIONS_H

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Global Variables
            //

            const extern struct luaL_Reg PluginVirtualMachineInternalFunctions[];

            //
            // Functions
            //

            bool IsPluginVirtualMachineInternalFuncion(const std::string& funcName);
        }
    }
}

#define RegisterPluginVirtualMachineInternalFunctions(vm)                                                                 \
    {                                                                                                                     \
        for (const luaL_Reg* func = wxbox::plugin::internal::PluginVirtualMachineInternalFunctions; func->func; func++) { \
            lua_register(vm->state, func->name, func->func);                                                              \
        }                                                                                                                 \
    }

#endif  // #ifndef __WXBOX_PLUGIN_INTERNAL_FUNCTIONS_H