#ifndef __WXBOX_PLUGIN_INTERNAL_MODULES_H
#define __WXBOX_PLUGIN_INTERNAL_MODULES_H

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Typedef
            //

            typedef bool (*ModuleMethodChecker)(const std::string& methodName);

            typedef struct ModuleMethodCheckerReg
            {
                const char*         moduleName;
                ModuleMethodChecker checker;
            } ModuleMethodCheckerReg;

            //
            // Global Variables
            //

            const extern struct luaL_Reg               PluginVirtualMachineInternalModules[];
            const extern struct ModuleMethodCheckerReg PluginVirtualMachineInternalModuleMethodCheckers[];

            //
            // Functions
            //

            bool IsPluginVirtualMachineInternalPublicModule(const std::string& moduleName);
            bool IsPluginVirtualMachineInternalModule(const std::string& moduleName);
            bool IsPluginVirtualMachineInternalModuleMethod(const std::string& moduleName, const std::string& methodName);
        }
    }
}

#define RegisterPluginVirtualMachineInternalModules(vm)                                                              \
    {                                                                                                                \
        for (const luaL_Reg* lib = wxbox::plugin::internal::PluginVirtualMachineInternalModules; lib->func; lib++) { \
            luaL_requiref(vm->state, lib->name, lib->func, 1);                                                       \
            lua_pop(vm->state, 1);                                                                                   \
        }                                                                                                            \
    }

#endif  // #ifndef __WXBOX_PLUGIN_INTERNAL_MODULES_H