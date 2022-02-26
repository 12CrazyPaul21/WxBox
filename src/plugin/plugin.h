#ifndef __WXBOX_PLUGIN_H
#define __WXBOX_PLUGIN_H

#include "config.h"

//
// Platform related headers
//

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>

#elif WXBOX_IN_MAC_OS

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

//
// C/C++ headers
//

#include <string>
#include <csetjmp>
#include <cassert>
#include <shared_mutex>
#include <condition_variable>
#include <deque>

//
// Third party headers
//

#include <lua.hpp>

//
// WxBox headers
//

#include <utils/common.h>

//
// Global variables
//

extern std::jmp_buf lua_painc_jmp;

//
// Macro
//

#define WXBOX_WECHAT_EXECUTOR_WXID "filehelper"

#define WXBOX_PLUGIN_FILE_EXT "lua"
#define WXBOX_PLUGIN_LIBS_PATH "libs"
#define WXBOX_PLUGIN_LUA_SEARCH_PATTERN "?.lua"

#if WXBOX_IN_WINDOWS_OS
#define WXBOX_PLUGIN_LUA_SO_SEARCH_PATTERN "?.dll"
#else
#define WXBOX_PLUGIN_LUA_SO_SEARCH_PATTERN "?.so"
#endif

#define WXBOX_PLUGIN_LONG_TASK_DEFAULT_TIMEOUT_MS 5000
#define WXBOX_PLUGINS_REFRESH_WAITING_TIME 1000

#define WXBOX_LUA_EXCEPTION_TRY \
    if (::setjmp(lua_painc_jmp) == 0)
#define WXBOX_LUA_EXCEPTION_CATCH else
#define WXBOX_LUA_EXCEPTION_THROW(x) \
    ::longjmp(lua_painc_jmp, (x))
#define WXBOX_LUA_PANIC_JMP_RESET()                         \
    {                                                       \
        ::memset(&lua_painc_jmp, 0, sizeof(lua_painc_jmp)); \
    }

#define RegisterPluginVirtualMachineTrait(TRAIT, TYPE, CONTAINER)             \
    namespace wxbox {                                                         \
        namespace plugin {                                                    \
            using CONTAINER##Ptr = std::shared_ptr<wxbox::plugin::CONTAINER>; \
        }                                                                     \
    }                                                                         \
    template<>                                                                \
    struct wxbox::plugin::TRAIT<TYPE>                                         \
    {                                                                         \
        using ContainerPtrType = wxbox::plugin::CONTAINER##Ptr;               \
    };

//
// WxBox plugin defind
//

namespace wxbox {
    namespace plugin {

        struct _PluginVirtualMachine;

        //
        // Host Event Model [plugin -> host]
        //

        enum class PluginLogLevel
        {
            Information = 0,
            Warning     = 1,
            Error       = 2
        };

        typedef struct _PluginLog
        {
            PluginLogLevel level;
            std::string    message;
        } PluginLog, *PPluginLog;

        using PluginLogPtr = std::shared_ptr<PluginLog>;

        enum class HostEventType
        {
            SendTextMesage = 0,
            Log,
            ClearCommandResultScreen,
            _TotalHostEventType
        };

        struct HostEventModel
        {
            HostEventType type;
            std::string   wxid;
            std::string   textMessage;
            PluginLogPtr  log;
        };

        using HostEventModelPtr = std::shared_ptr<HostEventModel>;

        //
        // Plugin Event Model [host -> plugin]
        //

        enum class PluginEventType
        {
            Load = 0,
            PreReload,
            UnLoad,
            ReceiveRawMessage,   // all raw messages
            ReceiveMessage,      // audio, video, file, picture, emoji ... messages
            ReceiveTextMessage,  // only test message(include plugin command statement)
            SendTextMessage,     // only before send 'text' message
            LoginWeChatEvent,    // wechat logged-in
            LogoutWeChatEvent,   // wechat logouted
            ExitWeChatEvent,     // wechat exit
            _TotalPluginEventType
        };

        struct PluginEventModel
        {
            PluginEventType type;
            uint32_t        messageType;
            std::string     wxid;
            std::string     message;
            std::string     chatroomTalkerWxid;
            void*           pData1;
            void*           pData2;
        };

        using PluginEventModelPtr = std::shared_ptr<PluginEventModel>;

        //
        // Plugin Context
        //

        typedef struct _WxBoxPluginContext
        {
            std::string name;
            std::string fullpath;
            std::time_t modifySecTimestamp;
            std::time_t mark;

            _WxBoxPluginContext()
              : name("")
              , fullpath("")
              , modifySecTimestamp(0)
              , mark(0)
            {
            }
        } WxBoxPluginContext, *PWxBoxPluginContext;

        //
        // Inner Methods
        //

        int __declare_plugin(lua_State* L);

        //
        // Methods
        //

        bool TriggerPluginOnLoad(_PluginVirtualMachine* vm, const std::string& pluginName);
        bool TriggerPluginOnUnLoad(_PluginVirtualMachine* vm, const std::string& pluginName);
        bool TriggerPluginOnPreReLoad(_PluginVirtualMachine* vm, const std::string& pluginName);
        bool TriggerPluginEvent(_PluginVirtualMachine* vm, const std::string& pluginName, PluginEventModelPtr pluginEvent);

        bool IsPluginEventName(const std::string& methodName);

        std::string       HostEventTypeToString(HostEventType type);
        HostEventModelPtr BuildHostEventModel();
        HostEventModelPtr CopyHostEventModel(HostEventModel* model);

        std::string         PluginEventTypeToString(PluginEventType type);
        PluginEventModelPtr BuildPluginEventModel();

        template<typename PluginModuleType, const char* TypeName>
        PluginModuleType* PushUserDataPtrToStack(lua_State* L, PluginModuleType* ptr)
        {
            static_assert(!std::is_pointer<PluginModuleType>(), "can not be a pointer type");

            if (!L) {
                return nullptr;
            }

            PluginModuleType** pptr = reinterpret_cast<PluginModuleType**>(lua_newuserdata(L, sizeof(PluginModuleType*)));
            if (!pptr) {
                return 0;
            }

            *pptr = ptr;

            luaL_getmetatable(L, TypeName);
            lua_setmetatable(L, -2);
            return *pptr;
        }

        template<typename PluginModuleType, const char* TypeName>
        PluginModuleType* PushUserDataToStack(lua_State* L)
        {
            static_assert(!std::is_pointer<PluginModuleType>(), "can not be a pointer type");

            if (!L) {
                return nullptr;
            }

            PluginModuleType** pptr = reinterpret_cast<PluginModuleType**>(lua_newuserdata(L, sizeof(PluginModuleType*)));
            if (!pptr) {
                return 0;
            }

            *pptr = new PluginModuleType();

            luaL_getmetatable(L, TypeName);
            lua_setmetatable(L, -2);
            return *pptr;
        }

        template<typename PluginModuleType, const char* TypeName>
        PluginModuleType* PushUserDataToStack(lua_State* L, PluginModuleType& object)
        {
            PluginModuleType* ptr = PushUserDataToStack<PluginModuleType, TypeName>(std::forward<decltype(L)>(L));
            if (!ptr) {
                return nullptr;
            }

            ptr->operator=(object);
            return ptr;
        }

        template<typename PluginModuleType, const char* TypeName>
        PluginModuleType* FetchUserDataPointer(lua_State* L)
        {
            static_assert(!std::is_pointer<PluginModuleType>(), "can not be a pointer type");

            if (!L) {
                return nullptr;
            }

            if (lua_type(L, 1) != LUA_TUSERDATA) {
                return nullptr;
            }

            auto pptr = reinterpret_cast<PluginModuleType**>(luaL_checkudata(L, 1, TypeName));
            if (!pptr) {
                return nullptr;
            }

            return *pptr;
        }
    }
}

//
// WxBox plugin internal modules
//

#include <plugin/internal/module/module_wxbox.h>
#include <plugin/internal/module/model_wechat_contact.h>
#include <plugin/internal/module/model_event.h>
#include <plugin/internal/module/model_host_event.h>

//
// WxBox plugin headers
//

#include <plugin/internal/plugin_internal_functions.h>
#include <plugin/internal/plugin_internal_modules.h>
#include <plugin/plugin_virtual_machine.h>
#include <plugin/plugin_command_parser.h>

//
// Short namespace
//

namespace wb_plugin          = wxbox::plugin;
namespace wb_plugin_internal = wxbox::plugin::internal;

#endif  // #ifndef __WXBOX_PLUGIN_H