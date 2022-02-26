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

void wxbot::WxBot::DispatchPluginReceiveRawWeChatMessage(wb_wx::WeChatMessageType type, wb_wx::PWeChatMessage message, bool sync)
{
    if (!message) {
        return;
    }

    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type        = wb_plugin::PluginEventType::ReceiveRawMessage;
    command->event->pData1      = message;
    command->event->messageType = (uint32_t)type;

    if (message->talker_wxid) {
        command->event->wxid = wb_string::ToString(message->talker_wxid);
    }
    if (message->message) {
        command->event->message = wb_string::ToString(message->message);
    }
    if (message->chatroom_talker_wxid) {
        command->event->chatroomTalkerWxid = wb_string::ToString(message->chatroom_talker_wxid);
    }

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginReceiveWeChatMessage(wb_wx::PWeChatMessage message, bool sync)
{
    if (!message) {
        return;
    }

    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type        = wb_plugin::PluginEventType::ReceiveMessage;
    command->event->pData1      = message;
    command->event->messageType = message->message_type;

    if (message->talker_wxid) {
        command->event->wxid = wb_string::ToString(message->talker_wxid);
    }
    if (message->message) {
        command->event->message = wb_string::ToString(message->message);
    }
    if (message->chatroom_talker_wxid) {
        command->event->chatroomTalkerWxid = wb_string::ToString(message->chatroom_talker_wxid);
    }

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginReceiveTextWeChatMessage(wb_wx::PWeChatMessage message, bool sync)
{
    if (!message) {
        return;
    }

    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type        = wb_plugin::PluginEventType::ReceiveTextMessage;
    command->event->pData1      = message;
    command->event->messageType = (uint32_t)wb_wx::WeChatMessageType::PLAINTEXT;

    if (message->talker_wxid) {
        command->event->wxid = wb_string::ToString(message->talker_wxid);
    }
    if (message->message) {
        command->event->message = wb_string::ToString(message->message);
    }
    if (message->chatroom_talker_wxid) {
        command->event->chatroomTalkerWxid = wb_string::ToString(message->chatroom_talker_wxid);
    }

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginSendTextWeChatMessage(wxbox::crack::wx::PWeChatWString wxid, wxbox::crack::wx::PWeChatWString message, bool sync)
{
    if (!wxid || !message || !wxid->str || !message->str) {
        return;
    }

    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type        = wb_plugin::PluginEventType::SendTextMessage;
    command->event->messageType = (uint32_t)wb_wx::WeChatMessageType::PLAINTEXT;
    command->event->pData1      = wxid;
    command->event->pData2      = message;
    command->event->wxid        = wb_string::ToString(wxid->str);
    command->event->message     = wb_string::ToString(message->str);

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginLoginWeChatMessage(bool sync)
{
    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type = wb_plugin::PluginEventType::LoginWeChatEvent;

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginLogoutWeChatMessage(bool sync)
{
    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type = wb_plugin::PluginEventType::LogoutWeChatEvent;

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
    }
}

void wxbot::WxBot::DispatchPluginExitWeChatMessage(bool sync)
{
    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    command->event = wb_plugin::BuildPluginEventModel();
    if (!command->event) {
        return;
    }

    command->event->type = wb_plugin::PluginEventType::ExitWeChatEvent;

    wb_plugin::PushPluginVirtualMachineCommand(command);
    if (sync) {
        command->signal.get_future().wait();
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

    auto event = pluginToHostEvent->hostEvent;

    switch (event->type) {
        case wb_plugin::HostEventType::Log: {
            if (!event->log) {
                break;
            }

            this->log((wxbox::WxBotLogLevel)event->log->level, event->log->message.c_str());
            break;
        }
    }
}