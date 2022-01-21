#include <plugin/plugin.h>

//
// Global variables
//

std::jmp_buf lua_painc_jmp;

static wb_plugin::PPluginVirtualMachine g_vm_signleton = nullptr;

static constexpr const char* ERROR_MESSAGE_MODULE_INVALID          = "the specified module is invalid";
static constexpr const char* ERROR_MESSAGE_MODULE_NOT_REGISTERED   = "the specified module is not registered";
static constexpr const char* ERROR_MESSAGE_METHOD_INVALID          = "the specified method is invalid";
static constexpr const char* ERROR_MESSAGE_CANT_CALL_EVENT_HANDLER = "cannot call the event handler of a module";

//
// Forward declaration
//

static int                                  PluginVirtualMachinePanicHandler(lua_State* L);
static wxbox::plugin::PPluginVirtualMachine BuildPluginVirtualMachine(wb_plugin::PPluginVirtualMachineStartupInfo startupInfo);
static void                                 ReleasePluginVirtualMachine(wxbox::plugin::PPPluginVirtualMachine ppvm);
static bool                                 InitPluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm);
static void                                 ClosePluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm);
static void                                 ResetPluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm);
static std::string                          PluginVirtualMachinePath(wxbox::plugin::PPluginVirtualMachine vm);
static std::string                          PluginVirtualMachineCPath(wxbox::plugin::PPluginVirtualMachine vm);
static void                                 AddPluginVirtualMachinePath(wxbox::plugin::PPluginVirtualMachine vm, const std::string& path);
static void                                 AddPluginVirtualMachineCPath(wxbox::plugin::PPluginVirtualMachine vm, const std::string& cpath);
static void                                 RemoveGlobalObject(wxbox::plugin::PPluginVirtualMachine vm, const std::string& objectName);
static bool                                 LoadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName);
static bool                                 UnLoadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName);
static void                                 UnLoadAllPlugin(wxbox::plugin::PPluginVirtualMachine vm);
static bool                                 ReloadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName);
static void                                 RefreshPlugins(wxbox::plugin::PPluginVirtualMachine vm);

//
// Functions
//

static int PluginVirtualMachinePanicHandler(lua_State* L)
{
    WXBOX_LUA_EXCEPTION_THROW(1);
    return 0;
}

static wxbox::plugin::PPluginVirtualMachine BuildPluginVirtualMachine(wb_plugin::PPluginVirtualMachineStartupInfo startupInfo)
{
    if (!startupInfo) {
        return nullptr;
    }

    wb_plugin::PPluginVirtualMachine vm = new wb_plugin::PluginVirtualMachine;
    vm->state                           = nullptr;
    vm->pluginPath                      = startupInfo->pluginPath;
    vm->longTaskTimeout                 = startupInfo->longTaskTimeout;
    vm->pluginsRefreshWaitingTime       = startupInfo->pluginsRefreshWaitingTime;
    vm->callback                        = startupInfo->callback;

    if (!vm->pluginPath.empty()) {
        vm->modulePath = wb_file::JoinPath(vm->pluginPath, WXBOX_PLUGIN_LIBS_PATH);
    }

    return vm;
}

static void ReleasePluginVirtualMachine(wxbox::plugin::PPPluginVirtualMachine ppvm)
{
    if (!ppvm || !*ppvm) {
        return;
    }

    delete *ppvm;
    *ppvm = nullptr;
}

static bool InitPluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!vm || vm->state) {
        return false;
    }

    // reset lua_panic_jmp
    WXBOX_LUA_PANIC_JMP_RESET();

    // new lua virtual machine state
    vm->state = luaL_newstate();
    if (!vm->state) {
        return false;
    }

    // open lua libraries
    luaL_openlibs(vm->state);

    // register global functions
    RegisterPluginVirtualMachineInternalFunctions(vm);

    // register internal libraries
    RegisterPluginVirtualMachineInternalModules(vm);

    // add module search path
    AddPluginVirtualMachinePath(vm, vm->pluginPath);
    AddPluginVirtualMachineCPath(vm, vm->modulePath);

    // add thirdparty module search path
    auto commonPath = wb_file::JoinPath(vm->pluginPath, "common");
    AddPluginVirtualMachinePath(vm, commonPath);
    AddPluginVirtualMachineCPath(vm, wb_file::JoinPath(commonPath, "libs"));

    // load all lua plugins
    RefreshPlugins(vm);

    // register at panic handler
    lua_atpanic(vm->state, PluginVirtualMachinePanicHandler);

    // init signal
    vm->exitSignal                     = std::promise<void>();
    vm->doneSignal                     = std::promise<void>();
    vm->exitFuture                     = vm->exitSignal.get_future();
    vm->doneFuture                     = vm->doneSignal.get_future();
    vm->pluginsRefreshTriggerTimestamp = 0;

    return true;
}

static void ClosePluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return;
    }

    // execute garbage collection
    lua_gc(vm->state, LUA_GCCOLLECT);

    // unload all plugins
    UnLoadAllPlugin(vm);

    // close lua context
    lua_close(vm->state);
    vm->state = nullptr;
}

static void ResetPluginVirtualMachine(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return;
    }

    ClosePluginVirtualMachine(vm);
    InitPluginVirtualMachine(vm);
}

static std::string PluginVirtualMachinePath(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return "";
    }

    lua_getglobal(vm->state, "package");
    lua_getfield(vm->state, -1, "path");

    const char* path = lua_tostring(vm->state, -1);
    if (path) {
        return path;
    }

    return "";
}

static std::string PluginVirtualMachineCPath(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return "";
    }

    lua_getglobal(vm->state, "package");
    lua_getfield(vm->state, -1, "cpath");

    const char* cpath = lua_tostring(vm->state, -1);
    if (cpath) {
        return cpath;
    }

    return "";
}

static void AddPluginVirtualMachinePath(wxbox::plugin::PPluginVirtualMachine vm, const std::string& path)
{
    if (!CheckPluginVirtualMachineValid(vm) || !wb_file::IsDirectory(path)) {
        return;
    }

    std::string p;

    lua_getglobal(vm->state, "package");
    lua_getfield(vm->state, -1, "path");

    const char* _p = lua_tostring(vm->state, -1);
    if (_p) {
        p.append(_p);
        p.append(";");
    }
    p.append(wb_file::JoinPath(path, WXBOX_PLUGIN_LUA_SEARCH_PATTERN));

    lua_pushstring(vm->state, p.c_str());
    lua_setfield(vm->state, -3, "path");

    lua_pop(vm->state, 2);

    return;
}

static void AddPluginVirtualMachineCPath(wxbox::plugin::PPluginVirtualMachine vm, const std::string& cpath)
{
    if (!CheckPluginVirtualMachineValid(vm) || !wb_file::IsDirectory(cpath)) {
        return;
    }

    std::string p;

    lua_getglobal(vm->state, "package");
    lua_getfield(vm->state, -1, "cpath");

    const char* _p = lua_tostring(vm->state, -1);
    if (_p) {
        p.append(_p);
        p.append(";");
    }
    p.append(wb_file::JoinPath(cpath, WXBOX_PLUGIN_LUA_SO_SEARCH_PATTERN));

    lua_pushstring(vm->state, p.c_str());
    lua_setfield(vm->state, -3, "cpath");

    lua_pop(vm->state, 2);

    return;
}

static void RemoveGlobalObject(wxbox::plugin::PPluginVirtualMachine vm, const std::string& objectName)
{
    if (!CheckPluginVirtualMachineValid(vm) || objectName.empty()) {
        return;
    }

    if (lua_getglobal(vm->state, objectName.c_str()) == LUA_TNIL) {
        lua_pop(vm->state, 1);
        return;
    }
    lua_pop(vm->state, 1);

    // set global object value to nil
    lua_pushnil(vm->state);
    lua_setglobal(vm->state, objectName.c_str());
}

static bool LoadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginFileName.empty()) {
        return false;
    }

    decltype(vm->plugins)& plugins = vm->plugins;

    if (plugins.find(pluginFileName) != plugins.end()) {
        return false;
    }

    std::string pluginName = wb_file::ExtractFileNameAndExt(pluginFileName).first;
    if (pluginName.empty()) {
        return false;
    }

    // check whether the plugin name has been defined in global
    if (lua_getglobal(vm->state, pluginName.c_str()) != LUA_TNIL) {
        lua_pop(vm->state, 1);
        return false;
    }
    lua_pop(vm->state, 1);

    // build a plugin context
    auto pluginContext = std::make_shared<wb_plugin::WxBoxPluginContext>();
    if (!pluginContext) {
        return false;
    }

    // config plugin context
    pluginContext->name               = pluginName;
    pluginContext->fullpath           = wb_file::JoinPath(vm->pluginPath, pluginFileName);
    pluginContext->modifySecTimestamp = wb_file::GetFileModifyTimestamp(pluginContext->fullpath);
    pluginContext->mark               = wb_process::GetCurrentTimestamp();

    // load plugin with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (luaL_dofile(vm->state, pluginContext->fullpath.c_str()) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);
        lua_pop(vm->state, 1);
        RemoveGlobalObject(vm, pluginName);
        return false;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    // trigger plugin "load" event
    if (!wb_plugin::TriggerPluginOnLoad(vm, pluginName)) {
        RemoveGlobalObject(vm, pluginName);
        return false;
    }

    plugins[pluginFileName] = pluginContext;
    return true;
}

static bool UnLoadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginFileName.empty()) {
        return false;
    }

    auto p = vm->plugins.find(pluginFileName);
    if (p == vm->plugins.end()) {
        return false;
    }

    auto pluginContext = p->second;
    if (!pluginContext) {
        vm->plugins.erase(p);
        return true;
    }

    // trigger plugin "unload" event
    wb_plugin::TriggerPluginOnUnLoad(vm, pluginContext->name);

    // remove plugin module
    RemoveGlobalObject(vm, pluginContext->name);
    vm->plugins.erase(p);
    return true;
}

static void UnLoadAllPlugin(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return;
    }

    for (auto p : vm->plugins) {
        auto pluginContext = p.second;
        if (!pluginContext) {
            continue;
        }

        // trigger plugin "unload" event
        wb_plugin::TriggerPluginOnUnLoad(vm, pluginContext->name);

        // remove plugin module
        RemoveGlobalObject(vm, pluginContext->name);
    }

    vm->plugins.clear();
}

static bool ReloadPlugin(wxbox::plugin::PPluginVirtualMachine vm, const std::string& pluginFileName)
{
    if (!CheckPluginVirtualMachineValid(vm) || pluginFileName.empty()) {
        return false;
    }

    auto p = vm->plugins.find(pluginFileName);
    if (p != vm->plugins.end()) {
        auto pluginContext = p->second;
        if (pluginContext) {
            // trigger plugin "prereload" event
            wb_plugin::TriggerPluginOnPreReLoad(vm, pluginContext->name);

            // remove plugin module
            RemoveGlobalObject(vm, pluginContext->name);
        }

        vm->plugins.erase(p);
    }

    return LoadPlugin(vm, pluginFileName);
}

static void RefreshPlugins(wxbox::plugin::PPluginVirtualMachine vm)
{
    if (!CheckPluginVirtualMachineValid(vm)) {
        return;
    }

    if (!wb_file::IsDirectory(vm->pluginPath)) {
        return;
    }

    // execute garbage collection
    lua_gc(vm->state, LUA_GCCOLLECT);

    // fetch all plugin's filename
    auto pluginLists = wb_file::ListFilesInDirectoryWithExt(vm->pluginPath, WXBOX_PLUGIN_FILE_EXT);

    // added¡¢ removed and modified plugin list
    std::vector<std::string>  addedPluginList;
    decltype(addedPluginList) removedPluginList;
    decltype(addedPluginList) modifiedPluginList;

    decltype(vm->plugins)& plugins   = vm->plugins;
    auto                   timestamp = wb_process::GetCurrentTimestamp();

    for (auto plugin : pluginLists) {
        // add new plugin
        if (plugins.find(plugin) == plugins.end()) {
            addedPluginList.push_back(plugin);
            continue;
        }

        //
        // check modified
        //

        auto pluginPath          = wb_file::JoinPath(vm->pluginPath, plugin);
        auto pluginFileTimestamp = wb_file::GetFileModifyTimestamp(pluginPath);
        auto pluginContext       = plugins[plugin];

        if (pluginFileTimestamp != pluginContext->modifySecTimestamp) {
            modifiedPluginList.push_back(plugin);
        }

        pluginContext->mark = timestamp;
    }

    // check removed
    for (auto p : plugins) {
        if (p.second->mark != timestamp) {
            removedPluginList.push_back(p.first);
        }
    }

    // unload plugin
    for (auto plugin : removedPluginList) {
        UnLoadPlugin(vm, plugin);
    }

    // reload plugin
    for (auto plugin : modifiedPluginList) {
        ReloadPlugin(vm, plugin);
    }

    // load plugin
    for (auto plugin : addedPluginList) {
        LoadPlugin(vm, plugin);
    }

    return;
}

static void PluginVirtualMachinePluginsChangeMonitor(wxbox::plugin::PPluginVirtualMachine vm, wb_file::FileChangeMonitorReport report)
{
    std::unique_lock<std::shared_mutex> lock(vm->rwmutex);
    vm->pluginsRefreshTriggerTimestamp = wb_process::GetCurrentTimestamp();
}

static inline bool PluginVirtualMachineTimeToStop(wxbox::plugin::PPluginVirtualMachine vm)
{
    assert(vm != nullptr && vm->state != nullptr);
    return vm->exitFuture.wait_for(std::chrono::milliseconds(1)) != std::future_status::timeout;
}

static inline void DispatchPluginVirtualMachineEvent(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginVirtualMachineEventPtr event)
{
    if (!vm || !vm->callback || !event) {
        return;
    }

    wb_process::async_task(vm->callback, event);
}

static inline void DispatchPluginEvent(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginEventModelPtr pluginEvent)
{
    assert(vm != nullptr && vm->state != nullptr);

    for (auto plugin : vm->plugins) {
        wb_plugin::TriggerPluginEvent(vm, plugin.second->name, pluginEvent);
    }
}

static bool IsModuleRegistered(wxbox::plugin::PPluginVirtualMachine vm, const std::string& moduleName)
{
    assert(vm != nullptr && vm->state != nullptr);

    if (moduleName.empty()) {
        return false;
    }

    if (wb_plugin_internal::IsPluginVirtualMachineInternalPublicModule(moduleName)) {
        return true;
    }

    for (auto plugin : vm->plugins) {
        if (!plugin.second->name.compare(moduleName)) {
            return true;
        }
    }

    return false;
}

static const char* VerifyCommandInfo(wxbox::plugin::PPluginVirtualMachine vm, wb_plugin::CommandExecuteInfoPtr commandInfo)
{
    assert(vm != nullptr && vm->state != nullptr && commandInfo != nullptr);

    if (commandInfo->type == wb_plugin::CommandMethodType::ModuleMethod) {
        if (!IsModuleRegistered(vm, commandInfo->moduleName)) {
            return ERROR_MESSAGE_MODULE_NOT_REGISTERED;
        }

        if (wb_plugin::IsPluginEventName(commandInfo->methodName)) {
            return ERROR_MESSAGE_CANT_CALL_EVENT_HANDLER;
        }

        if (lua_getglobal(vm->state, commandInfo->moduleName.c_str()) != LUA_TTABLE) {
            lua_pop(vm->state, 1);
            return ERROR_MESSAGE_MODULE_INVALID;
        }

        if (lua_getfield(vm->state, -1, commandInfo->methodName.c_str()) != LUA_TFUNCTION) {
            lua_pop(vm->state, 2);
            return ERROR_MESSAGE_METHOD_INVALID;
        }
        lua_pop(vm->state, 2);
    }
    else {
        if (lua_getglobal(vm->state, commandInfo->methodName.c_str()) != LUA_TFUNCTION) {
            lua_pop(vm->state, 1);
            return ERROR_MESSAGE_METHOD_INVALID;
        }
        lua_pop(vm->state, 1);
    }

    return nullptr;
}

static void ExecuteCommand(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::CommandParseResultPtr parseResult, bool fromFilehelper)
{
    assert(vm != nullptr && vm->state != nullptr);
    if (!parseResult) {
        return;
    }

    // build result event
    auto event = wb_plugin::BuildPluginVirtualMachineEvent<wb_plugin::PluginVirtualMachineEventType::ExecuteResult>();
    if (!event) {
        return;
    }
    event->fromFilehelper = fromFilehelper;

    // check parse result
    if (parseResult->status != wb_plugin::CommandParseStatus::Success || !parseResult->commandInfo) {
        event->status = false;
        event->error  = parseResult->error;
        DispatchPluginVirtualMachineEvent(vm, event);
        return;
    }

    // verify command info
    if (auto errorMsg = VerifyCommandInfo(vm, parseResult->commandInfo)) {
        event->status = false;
        event->error  = std::string("runtime error : ") + errorMsg;
        DispatchPluginVirtualMachineEvent(vm, event);
        return;
    }

    //
    // execute command
    //

    auto commandInfo = parseResult->commandInfo;
    auto pop         = 0;
    auto entry_top   = lua_gettop(vm->state);

    // push message to lua stack
    if (commandInfo->type == wb_plugin::CommandMethodType::ModuleMethod) {
        lua_getglobal(vm->state, commandInfo->moduleName.c_str());
        lua_getfield(vm->state, -1, commandInfo->methodName.c_str());
        pop++;
    }
    else {
        lua_getglobal(vm->state, commandInfo->methodName.c_str());
    }

    // push parameters to lua stack
    for (auto& parameter : commandInfo->argLists) {
        switch (parameter.type) {
            case wb_plugin::CommandParameterType::NumeralLiteral:
                lua_pushnumber(vm->state, LUA_NUMBER(parameter.numeralval));
                break;
            case wb_plugin::CommandParameterType::StringLiteral:
                lua_pushstring(vm->state, parameter.strval.c_str());
                break;
            case wb_plugin::CommandParameterType::BooleanLiteral:
                lua_pushboolean(vm->state, parameter.boolval);
                break;
        }
    }

    // execute command with timeout
    BEGIN_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);
    if (lua_pcall(vm->state, commandInfo->argLists.size(), LUA_MULTRET, 0) != LUA_OK) {
        CANCEL_PLUGIN_VIRTUAL_MACHINE_LONG_TASK(vm);

        if (const char* error = luaL_checkstring(vm->state, -1)) {
            event->error = std::string("runtime error : ") + error;
        }
        else {
            event->error = std::string("runtime error : lua_pcall is failed");
        }
        event->status = false;
        DispatchPluginVirtualMachineEvent(vm, event);

        lua_pop(vm->state, pop + 1);
        return;
    }
    END_PLUGIN_VIRTUAL_MACHINE_LONG_TASK_WITH_TIMEOUT(vm);

    // collect results
    for (auto i = entry_top + pop + 1, top = lua_gettop(vm->state); i <= top; i++) {
        auto result = std::make_shared<wb_plugin::CommandExecuteResult>();
        if (!result) {
            break;
        }

        switch (lua_type(vm->state, i)) {
            case LUA_TNONE:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ none ]";
                break;
            case LUA_TNIL:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ nil ]";
                break;
            case LUA_TBOOLEAN:
                result->type    = wb_plugin::CommandExecuteResultType::Boolean;
                result->boolval = lua_toboolean(vm->state, i);
                break;
            case LUA_TLIGHTUSERDATA:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ tlightuserdata ]";
                break;
            case LUA_TNUMBER:
                result->type = wb_plugin::CommandExecuteResultType::Number;
                if (lua_isinteger(vm->state, i)) {
                    result->numeralval = double(lua_tointeger(vm->state, i));
                }
                else {
                    result->numeralval = lua_tonumber(vm->state, i);
                }
                break;
            case LUA_TSTRING:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = lua_tostring(vm->state, i);
                break;
            case LUA_TTABLE:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ table ]";
                break;
            case LUA_TFUNCTION:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ function ]";
                break;
            case LUA_TUSERDATA:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ userdata ]";
                break;
            case LUA_TTHREAD:
                result->type   = wb_plugin::CommandExecuteResultType::String;
                result->strval = "[ thread ]";
                break;
        }

        event->results.push_back(result);
    }

    // balance stack
    lua_pop(vm->state, lua_gettop(vm->state) - entry_top);

    // dispatch command result event
    event->status = true;
    event->error  = "";
    DispatchPluginVirtualMachineEvent(vm, event);
}

static void HandleEvalCommand(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginVirtualMachineCommandEvalPtr command)
{
    assert(vm != nullptr && vm->state != nullptr);
    if (!command) {
        return;
    }

    ExecuteCommand(vm, wb_plugin::ParseCommandStatement(command->command.c_str()), false);
}

static void HandleReceiveWxChatTextMessageCommand(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginVirtualMachineCommandReceiveWxChatTextMessagePtr command)
{
    assert(vm != nullptr && vm->state != nullptr);
    if (!command) {
        return;
    }

    if (!command->wxid.compare(WXBOX_WECHAT_EXECUTOR_WXID)) {
        // parse command statement
        auto parseResult = wb_plugin::ParseCommandStatement(command->textMessage.c_str());
        if (parseResult && parseResult->status != wb_plugin::CommandParseStatus::NotCommand) {
            ExecuteCommand(vm, parseResult, true);
            return;
        }
    }

    // dispatch event to all plugins
    auto pluginEvent         = wb_plugin::BuildPluginEventModel();
    pluginEvent->type        = wb_plugin::PluginEventType::ReceiveTextMessage;
    pluginEvent->wxid        = command->wxid;
    pluginEvent->textMessage = command->textMessage;
    DispatchPluginEvent(vm, pluginEvent);
}

static void HandleSendWxChatTextMessageCommand(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginVirtualMachineCommandSendWxChatTextMessagePtr command)
{
    assert(vm != nullptr && vm->state != nullptr);

    if (!command) {
        return;
    }

    // dispatch event to all plugins
    auto pluginEvent         = wb_plugin::BuildPluginEventModel();
    pluginEvent->type        = wb_plugin::PluginEventType::SendTextMessage;
    pluginEvent->wxid        = command->wxid;
    pluginEvent->textMessage = command->textMessage;
    DispatchPluginEvent(vm, pluginEvent);
}

static void HandlePluginVirtualMachineCommand(wxbox::plugin::PPluginVirtualMachine vm, wxbox::plugin::PluginVirtualMachineCommandPtr command)
{
    assert(vm != nullptr && vm->state != nullptr && command != nullptr);

    switch (command->type) {
        case wb_plugin::PluginVirtualMachineCommandType::GC:
            lua_gc(vm->state, LUA_GCCOLLECT);
            break;
        case wb_plugin::PluginVirtualMachineCommandType::Eval:
            HandleEvalCommand(vm, wb_plugin::CastPluginVirtualMachineCommandPtr<wb_plugin::PluginVirtualMachineCommandType::Eval>(command));
            break;
        case wb_plugin::PluginVirtualMachineCommandType::ReceiveWxChatTextMessage:
            HandleReceiveWxChatTextMessageCommand(vm, wb_plugin::CastPluginVirtualMachineCommandPtr<wb_plugin::PluginVirtualMachineCommandType::ReceiveWxChatTextMessage>(command));
            break;
        case wb_plugin::PluginVirtualMachineCommandType::SendWxChatTextMessage:
            HandleSendWxChatTextMessageCommand(vm, wb_plugin::CastPluginVirtualMachineCommandPtr<wb_plugin::PluginVirtualMachineCommandType::SendWxChatTextMessage>(command));
            break;
    }

    command->signal.set_value();
}

static void PluginVirtualMachineMessageLoop(wxbox::plugin::PPluginVirtualMachine vm)
{
    assert(vm != nullptr && vm->state != nullptr);

    wb_plugin::PluginVirtualMachineCommandPtr command;

    for (;;) {
        {
            std::unique_lock<std::shared_mutex> lock(vm->rwmutex);
            bool                                timeout = !vm->cv.wait_for(
                lock, std::chrono::milliseconds(50), [vm] {
                    return vm->queue.size() || PluginVirtualMachineTimeToStop(vm);
                });

            // check plugin virtual machine stop
            if (PluginVirtualMachineTimeToStop(vm)) {
                break;
            }

            // check refresh plugins
            if (timeout && vm->pluginsRefreshTriggerTimestamp != 0) {
                auto delta = wb_process::GetCurrentTimestamp() - vm->pluginsRefreshTriggerTimestamp;
                if (delta >= vm->pluginsRefreshWaitingTime) {
                    RefreshPlugins(vm);
                    vm->pluginsRefreshTriggerTimestamp = 0;
                }
            }

            if (vm->queue.empty()) {
                continue;
            }

            command = std::move(vm->queue.front());
            vm->queue.pop_front();
        }

        // handle
        HandlePluginVirtualMachineCommand(vm, command);
    }
}

static void PluginVirtualMachineRoutine(wxbox::plugin::PPluginVirtualMachine vm)
{
    assert(vm != nullptr && vm->state != nullptr);
    PluginVirtualMachineMessageLoop(vm);
    vm->doneSignal.set_value();
}

bool wxbox::plugin::StartPluginVirtualMachine(PPluginVirtualMachineStartupInfo startupInfo)
{
    if (!startupInfo || ::g_vm_signleton) {
        return false;
    }

    // build plugin virtual machine
    ::g_vm_signleton = BuildPluginVirtualMachine(startupInfo);
    if (!::g_vm_signleton) {
        return false;
    }

    // init plugin virtual machine
    if (!InitPluginVirtualMachine(::g_vm_signleton)) {
        ReleasePluginVirtualMachine(&::g_vm_signleton);
        return false;
    }

    // running plugin virtual machine
    ::g_vm_signleton->worker = std::move(std::thread(std::bind(&PluginVirtualMachineRoutine, ::g_vm_signleton)));
    ::g_vm_signleton->worker.detach();

    // start plugin folder file change monitor
    wb_file::OpenFolderFilesChangeMonitor(::g_vm_signleton->pluginPath, std::bind(&PluginVirtualMachinePluginsChangeMonitor, ::g_vm_signleton, std::placeholders::_1));

    return true;
}

void wxbox::plugin::StopPluginVirtualMachine()
{
    if (!::g_vm_signleton) {
        return;
    }

    wb_file::CloseFolderFilesChangeMonitor(::g_vm_signleton->pluginPath);
    ::g_vm_signleton->exitSignal.set_value();
    ::g_vm_signleton->cv.notify_one();
    ::g_vm_signleton->doneFuture.wait();
    ClosePluginVirtualMachine(::g_vm_signleton);
    ReleasePluginVirtualMachine(&::g_vm_signleton);
}

void wxbox::plugin::ExecutePluginVirtualMachineGC()
{
    PushPluginVirtualMachineCommand(BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::GC>());
}

std::string wxbox::plugin::GetPluginVirtualMachineStorageRoot()
{
    if (!::g_vm_signleton) {
        return "";
    }

    return wb_file::JoinPath(g_vm_signleton->pluginPath, "storage");
}

bool wxbox::plugin::PushPluginVirtualMachineCommandSync(PluginVirtualMachineCommandPtr command)
{
    if (!::g_vm_signleton) {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(::g_vm_signleton->rwmutex);
    ::g_vm_signleton->queue.emplace_back(command);
    return true;
}

bool wxbox::plugin::PushPluginVirtualMachineCommand(PluginVirtualMachineCommandPtr command)
{
    if (!::g_vm_signleton) {
        return false;
    }

    wb_process::async_task(PushPluginVirtualMachineCommandSync, command);
    return true;
}

void wxbox::plugin::DispatchPluginToHostEvent(wxbox::plugin::HostEventModelPtr hostEvent)
{
    if (!::g_vm_signleton || !::g_vm_signleton->callback || !hostEvent) {
        return;
    }

    auto event = wb_plugin::BuildPluginVirtualMachineEvent<wb_plugin::PluginVirtualMachineEventType::PluginToHost>();
    if (!event) {
        return;
    }

    event->hostEvent = hostEvent;
    wb_process::async_task(::g_vm_signleton->callback, event);
}

void wxbox::plugin::internal::PluginVirtualMachineHookHandler(lua_State* L, lua_Debug* debugInfo)
{
    lua_sethook(L, NULL, 0, 0);
    luaL_error(L, "plugin virtual machine long task timeout");
}