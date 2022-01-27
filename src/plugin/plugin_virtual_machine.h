#ifndef __WXBOX_PLUGIN_VIRTUAL_MACHINE_H
#define __WXBOX_PLUGIN_VIRTUAL_MACHINE_H

namespace wxbox {
    namespace plugin {

        //
        // Command
        //

        enum class PluginVirtualMachineCommandType
        {
            Unknown = 0,
            GC,
            Eval,
            ReceiveWxChatTextMessage,
            SendWxChatTextMessage
        };

        struct PluginVirtualMachineCommand
        {
            PluginVirtualMachineCommandType type;
            std::promise<void>              signal;
        };

        struct PluginVirtualMachineCommandEval : public PluginVirtualMachineCommand
        {
            std::string command;
        };

        struct PluginVirtualMachineCommandReceiveWxChatTextMessage : public PluginVirtualMachineCommand
        {
            std::string wxid;
            std::string textMessage;
        };

        struct PluginVirtualMachineCommandSendWxChatTextMessage : public PluginVirtualMachineCommand
        {
            std::string wxid;
            std::string textMessage;
        };

        template<PluginVirtualMachineCommandType type>
        struct PluginVirtualMachineCommandTypeTrait
        {
            using ContainerPtrType = std::shared_ptr<PluginVirtualMachineCommand>;
        };

        using PluginVirtualMachineCommandPtr = std::shared_ptr<PluginVirtualMachineCommand>;

        //
        // Command Execute Result
        //

        enum class CommandExecuteResultType
        {
            Empty = 0,
            Number,
            String,
            Boolean
        };

        struct CommandExecuteResult
        {
            CommandExecuteResultType type;
            double                   numeralval;
            std::string              strval;
            bool                     boolval;
        };

        using CommandExecuteResultPtr = std::shared_ptr<CommandExecuteResult>;

        //
        // Event
        //

        enum class PluginVirtualMachineEventType
        {
            Unknown = 0,
            ExecuteResult,
            PluginToHost
        };

        struct PluginVirtualMachineEvent
        {
            PluginVirtualMachineEventType type;
        };

        struct PluginVirtualMachineExecuteResultEvent : public PluginVirtualMachineEvent
        {
            bool                                 status;
            std::string                          error;
            std::vector<CommandExecuteResultPtr> results;
            bool                                 fromFilehelper;
        };

        struct PluginVirtualMachinePluginToHostEvent : public PluginVirtualMachineEvent
        {
            HostEventModelPtr hostEvent;
        };

        template<PluginVirtualMachineEventType type>
        struct PluginVirtualMachineEventTypeTrait
        {
            using ContainerPtrType = std::shared_ptr<PluginVirtualMachineEvent>;
        };

        using PluginVirtualMachineEventPtr = std::shared_ptr<PluginVirtualMachineEvent>;
        using PluginVirtualMachineCallback = std::function<void(PluginVirtualMachineEventPtr)>;

        //
        // Plugin Virtual Machine
        //

        typedef struct _PluginVirtualMachineStartupInfo
        {
            std::string                  pluginPath;
            std::time_t                  longTaskTimeout;
            std::time_t                  pluginsRefreshWaitingTime;
            PluginVirtualMachineCallback callback;

            /*               <<< stub >>>               */
            /* WxBotContext*              wxbotContext; */

            _PluginVirtualMachineStartupInfo()
              : pluginPath("")
              , longTaskTimeout(WXBOX_PLUGIN_LONG_TASK_DEFAULT_TIMEOUT_MS)
              , pluginsRefreshWaitingTime(WXBOX_PLUGINS_REFRESH_WAITING_TIME)
              , callback(nullptr)
            {
            }
        } PluginVirtualMachineStartupInfo, *PPluginVirtualMachineStartupInfo;

        typedef struct _PluginVirtualMachine
        {
            lua_State*                                                           state;
            std::string                                                          pluginPath;
            std::string                                                          modulePath;
            std::unordered_map<std::string, std::shared_ptr<WxBoxPluginContext>> plugins;
            std::time_t                                                          longTaskTimeout;
            std::time_t                                                          pluginsRefreshWaitingTime;
            std::atomic<std::time_t>                                             pluginsRefreshTriggerTimestamp;
            PluginVirtualMachineCallback                                         callback;
            std::thread                                                          worker;
            std::deque<PluginVirtualMachineCommandPtr>                           queue;
            std::shared_mutex                                                    rwmutex;
            std::condition_variable_any                                          cv;
            std::promise<void>                                                   exitSignal;
            std::promise<void>                                                   doneSignal;
            std::future<void>                                                    exitFuture;
            std::future<void>                                                    doneFuture;
        } PluginVirtualMachine, *PPluginVirtualMachine, **PPPluginVirtualMachine;

        //
        // Methods
        //

        bool StartPluginVirtualMachine(PPluginVirtualMachineStartupInfo startupInfo);
        void StopPluginVirtualMachine();

        void        ExecutePluginVirtualMachineGC();
        std::string GetPluginVirtualMachineStorageRoot();

        template<PluginVirtualMachineCommandType type>
        auto BuildPluginVirtualMachineCommand() -> typename PluginVirtualMachineCommandTypeTrait<type>::ContainerPtrType
        {
            switch (type) {
                case wb_plugin::PluginVirtualMachineCommandType::Unknown:
                    return nullptr;
                default: {
                    auto ptr  = std::make_shared<PluginVirtualMachineCommandTypeTrait<type>::ContainerPtrType::element_type>();
                    ptr->type = type;
                    return ptr;
                }
            }
            return nullptr;
        }

        template<PluginVirtualMachineCommandType type>
        auto CastPluginVirtualMachineCommandPtr(PluginVirtualMachineCommandPtr ptr) -> typename PluginVirtualMachineCommandTypeTrait<type>::ContainerPtrType
        {
            switch (type) {
                case wb_plugin::PluginVirtualMachineCommandType::Unknown:
                    return nullptr;
                default: {
                    return std::static_pointer_cast<wb_plugin::PluginVirtualMachineCommandTypeTrait<type>::ContainerPtrType::element_type>(ptr);
                }
            }
            return nullptr;
        }

        template<PluginVirtualMachineEventType type>
        auto BuildPluginVirtualMachineEvent() -> typename PluginVirtualMachineEventTypeTrait<type>::ContainerPtrType
        {
            switch (type) {
                case wb_plugin::PluginVirtualMachineEventType::Unknown:
                    return nullptr;
                default: {
                    auto ptr  = std::make_shared<PluginVirtualMachineEventTypeTrait<type>::ContainerPtrType::element_type>();
                    ptr->type = type;
                    return ptr;
                }
            }
            return nullptr;
        }

        template<PluginVirtualMachineEventType type>
        auto CastPluginVirtualMachineEventPtr(PluginVirtualMachineEventPtr ptr) -> typename PluginVirtualMachineEventTypeTrait<type>::ContainerPtrType
        {
            switch (type) {
                case wb_plugin::PluginVirtualMachineEventType::Unknown:
                    return nullptr;
                default: {
                    return std::static_pointer_cast<wb_plugin::PluginVirtualMachineEventTypeTrait<type>::ContainerPtrType::element_type>(ptr);
                }
            }
            return nullptr;
        }

        bool PushPluginVirtualMachineCommandSync(PluginVirtualMachineCommandPtr command);
        bool PushPluginVirtualMachineCommand(PluginVirtualMachineCommandPtr command);

        void DispatchPluginToHostEvent(HostEventModelPtr hostEvent);

        namespace internal {
            void PluginVirtualMachineHookHandler(lua_State* L, lua_Debug* debugInfo);
        }
    }
}

#define CheckPluginVirtualMachineValid(vm) (vm && vm->state)

#define BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm)                                                            \
    {                                                                                                                      \
        auto               __task_begin_timestamp = wb_process::GetCurrentTimestamp();                                     \
        std::promise<void> __task_complete_signal;                                                                         \
                                                                                                                           \
        auto __task_async_future = std::async(                                                                             \
            std::launch::async, [__task_begin_timestamp, vm](std::future<void> taskFuture) {                               \
                for (;;) {                                                                                                 \
                    if (taskFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::timeout) {               \
                        /* task complete */                                                                                \
                        break;                                                                                             \
                    }                                                                                                      \
                                                                                                                           \
                    if (wb_process::GetCurrentTimestamp() - __task_begin_timestamp >= vm->longTaskTimeout) {               \
                        /* timeout */                                                                                      \
                        lua_sethook(vm->state, wxbox::plugin::internal::PluginVirtualMachineHookHandler, LUA_MASKLINE, 0); \
                        break;                                                                                             \
                    }                                                                                                      \
                }                                                                                                          \
            },                                                                                                             \
            __task_complete_signal.get_future());

#define CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm) \
    {                                               \
        __task_complete_signal.set_value();         \
        __task_async_future.wait();                 \
        lua_sethook(vm->state, NULL, 0, 0);         \
    }

#define END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm) \
    CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);              \
    }

#define RegisterPluginVirtualMachineCommandType(TYPE, CONTAINER) RegisterPluginVirtualMachineTrait(PluginVirtualMachineCommandTypeTrait, TYPE, CONTAINER)
#define RegisterPluginVirtualMachineEventType(TYPE, CONTAINER) RegisterPluginVirtualMachineTrait(PluginVirtualMachineEventTypeTrait, TYPE, CONTAINER)

// register plugin virtual machine command type
RegisterPluginVirtualMachineCommandType(wxbox::plugin::PluginVirtualMachineCommandType::Unknown, PluginVirtualMachineCommand);
RegisterPluginVirtualMachineCommandType(wxbox::plugin::PluginVirtualMachineCommandType::GC, PluginVirtualMachineCommand);
RegisterPluginVirtualMachineCommandType(wxbox::plugin::PluginVirtualMachineCommandType::Eval, PluginVirtualMachineCommandEval);
RegisterPluginVirtualMachineCommandType(wxbox::plugin::PluginVirtualMachineCommandType::ReceiveWxChatTextMessage, PluginVirtualMachineCommandReceiveWxChatTextMessage);
RegisterPluginVirtualMachineCommandType(wxbox::plugin::PluginVirtualMachineCommandType::SendWxChatTextMessage, PluginVirtualMachineCommandSendWxChatTextMessage);

// register plugin virtual machine event type
RegisterPluginVirtualMachineEventType(wxbox::plugin::PluginVirtualMachineEventType::Unknown, PluginVirtualMachineEvent);
RegisterPluginVirtualMachineEventType(wxbox::plugin::PluginVirtualMachineEventType::ExecuteResult, PluginVirtualMachineExecuteResultEvent);
RegisterPluginVirtualMachineEventType(wxbox::plugin::PluginVirtualMachineEventType::PluginToHost, PluginVirtualMachinePluginToHostEvent);

#endif  // #ifndef __WXBOX_PLUGIN_VIRTUAL_MACHINE_H