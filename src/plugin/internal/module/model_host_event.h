#ifndef __WXBOX_PLUGIN_MODEL_HOST_EVENT_H
#define __WXBOX_PLUGIN_MODEL_HOST_EVENT_H

const char HOST_EVENT_MODEL_NAME[] = "wxbox_host_event";

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Global Variables
            //

            const extern struct luaL_Reg HostEventModelMethods[];
            const extern struct luaL_Reg HostEventModelObjectMethods[];

            //
            // Functions
            //

            int  __luaopen_wxbox_host_event_model(lua_State* L);
            bool IsHostEventModelMethod(const std::string& methodName);
        }
    }
}

#endif  // #ifndef __WXBOX_PLUGIN_MODEL_HOST_EVENT_H