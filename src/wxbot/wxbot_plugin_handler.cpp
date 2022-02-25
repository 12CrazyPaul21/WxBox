#include <wxbot.hpp>

//
// Plugin API
//

void wxbot::WxBot::ExecutePluginScript(const std::string& statement)
{
    auto command     = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::Eval>();
    command->command = statement;
    wb_plugin::PushPluginVirtualMachineCommand(command);
    return;
}

void wxbot::WxBot::DispatchPluginResult(const std::string& result, bool fromFilehelper)
{
    if (fromFilehelper) {
        SendTextMessageToFileHelper(result);
    }
    else {
        ResponseExecutePluginResult(result);
    }
}

void wxbot::WxBot::DispatchPluginErrorReport(const std::string& errorMsg, bool fromFilehelper)
{
    if (fromFilehelper) {
        SendTextMessageToFileHelper(errorMsg);
    }
    else {
        log(wxbox::WxBotLogLevel::Error, errorMsg.c_str());
    }
}

//
// Plugin Handler
//

void wxbot::WxBot::PluginExecuteResultEventHandler(const wb_plugin::PluginVirtualMachineExecuteResultEventPtr& resultEvent)
{
    if (!resultEvent) {
        return;
    }

    if (!resultEvent->status) {
        DispatchPluginErrorReport(resultEvent->error, resultEvent->fromFilehelper);
        return;
    }

    if (!resultEvent->results.size()) {
        return;
    }

    //
    // generate results text
    //

    std::stringstream resultText;

    for (auto result : resultEvent->results) {
        switch (result->type) {
            case wb_plugin::CommandExecuteResultType::Number:
                resultText << result->numeralval << std::endl;
                break;
            case wb_plugin::CommandExecuteResultType::String:
                resultText << result->strval << std::endl;
                break;
            case wb_plugin::CommandExecuteResultType::Boolean:
                resultText << std::boolalpha << result->boolval << std::endl;
                break;
        }
    }

    DispatchPluginResult(resultText.str(), resultEvent->fromFilehelper);
}

void wxbot::WxBot::PluginToHostEventHandler(const wb_plugin::PluginVirtualMachinePluginToHostEventPtr& pluginToHostEvent)
{
    if (!pluginToHostEvent || !pluginToHostEvent->hostEvent) {
        return;
    }
}