#ifndef __WXBOX_PLUGIN_MODEL_WECHAT_CONTACT_H
#define __WXBOX_PLUGIN_MODEL_WECHAT_CONTACT_H

const char WXCONTACT_MODEL_NAME[] = "wxcontact";

namespace wxbox {
    namespace plugin {
        namespace internal {

            //
            // Define Model
            //

            struct WxContactModel
            {
                std::string wxid;
                std::string username;
                std::string nickname;
                std::string remark;
            };

            //
            // Global Variables
            //

            const extern struct luaL_Reg WxContactModelMethods[];
            const extern struct luaL_Reg WxContactModelObjectMethods[];

            //
            // Functions
            //

            int  __luaopen_wx_contact_model(lua_State* L);
            bool IsWxContactModelMethod(const std::string& methodName);
        }
    }
}

#endif  // #ifndef __WXBOX_PLUGIN_MODEL_WECHAT_CONTACT_H