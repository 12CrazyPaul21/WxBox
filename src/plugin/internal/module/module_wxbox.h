#ifndef __WXBOX_PLUGIN_MODULE_WXBOX_H
#define __WXBOX_PLUGIN_MODULE_WXBOX_H

const char WXBOX_MODULE_NAME[] = "wxbox";

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Global Variables
            //

            const extern struct luaL_Reg WxBoxModuleMethods[];

            //
            // Functions
            //

            int  __luaopen_wxbox_module(lua_State* L);
            bool IsWxBoxModuleMethod(const std::string& methodName);
        }
    }
}

#endif  // #ifndef __WXBOX_PLUGIN_MODULE_WXBOX_H