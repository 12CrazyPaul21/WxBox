#include <plugin/plugin.h>

//
// Global Variables
//

static const char* PUBLIC_MODULE_LIST[] = {
    WXBOX_MODULE_NAME,
};

//
// export plugin virtual machine internal modules
//

const struct luaL_Reg wxbox::plugin::internal::PluginVirtualMachineInternalModules[] = {
    {WXBOX_MODULE_NAME, wb_plugin_internal::__luaopen_wxbox_module},
    {WXCONTACT_MODEL_NAME, wb_plugin_internal::__luaopen_wx_contact_model},
    {EVENT_MODEL_NAME, wb_plugin_internal::__luaopen_wxbox_plugin_event_model},
    {HOST_EVENT_MODEL_NAME, wb_plugin_internal::__luaopen_wxbox_host_event_model},
    {NULL, NULL},
};

const struct wxbox::plugin::internal::ModuleMethodCheckerReg wxbox::plugin::internal::PluginVirtualMachineInternalModuleMethodCheckers[] = {
    {WXBOX_MODULE_NAME, wb_plugin_internal::IsWxBoxModuleMethod},
    {WXCONTACT_MODEL_NAME, wb_plugin_internal::IsWxContactModelMethod},
    {EVENT_MODEL_NAME, wb_plugin_internal::IsPluginEventModelMethod},
    {HOST_EVENT_MODEL_NAME, wb_plugin_internal::IsHostEventModelMethod},
    {NULL, NULL},
};

//
// export function
//

bool wxbox::plugin::internal::IsPluginVirtualMachineInternalPublicModule(const std::string& moduleName)
{
    for (auto public_module : PUBLIC_MODULE_LIST) {
        if (!moduleName.compare(public_module)) {
            return true;
        }
    }
    return false;
}

bool wxbox::plugin::internal::IsPluginVirtualMachineInternalModule(const std::string& moduleName)
{
    for (const luaL_Reg* func = PluginVirtualMachineInternalModules; func->func; func++) {
        if (!moduleName.compare(func->name)) {
            return true;
        }
    }
    return false;
}

bool wxbox::plugin::internal::IsPluginVirtualMachineInternalModuleMethod(const std::string& moduleName, const std::string& methodName)
{
    for (const ModuleMethodCheckerReg* checker = PluginVirtualMachineInternalModuleMethodCheckers; checker->checker; checker++) {
        if (!moduleName.compare(checker->moduleName)) {
            return checker->checker(methodName);
        }
    }
    return false;
}