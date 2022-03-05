#ifndef __WXBOX_PLUGIN_MODEL_EVENT_H
#define __WXBOX_PLUGIN_MODEL_EVENT_H

const char EVENT_MODEL_NAME[] = "wxbox_plugin_event";

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Global Variables
            //

            const extern struct luaL_Reg PluginEventModelMethods[];
            const extern struct luaL_Reg PluginEventModelObjectMethods[];

            //
            // Functions
            //

            int  __luaopen_wxbox_plugin_event_model(lua_State* L);
            bool IsPluginEventModelMethod(const std::string& methodName);
        }
    }
}

#endif  // #ifndef __WXBOX_PLUGIN_MODEL_EVENT_H