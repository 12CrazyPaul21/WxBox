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

    if (message->talker_wxid && message->talker_wxid_length) {
        command->event->wxid = wb_string::ToUtf8String(message->talker_wxid);
    }
    if (message->message && message->message_length) {
        command->event->message = wb_string::ToUtf8String(message->message);
    }
    if (message->chatroom_talker_wxid && message->chatroom_talker_wxid_length) {
        command->event->chatroomTalkerWxid = wb_string::ToUtf8String(message->chatroom_talker_wxid);
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

    if (message->talker_wxid && message->talker_wxid_length) {
        command->event->wxid = wb_string::ToUtf8String(message->talker_wxid);
    }
    if (message->message && message->message_length) {
        command->event->message = wb_string::ToUtf8String(message->message);
    }
    if (message->chatroom_talker_wxid && message->chatroom_talker_wxid_length) {
        command->event->chatroomTalkerWxid = wb_string::ToUtf8String(message->chatroom_talker_wxid);
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

    if (message->talker_wxid && message->talker_wxid_length) {
        command->event->wxid = wb_string::ToUtf8String(message->talker_wxid);
    }
    if (message->message && message->message_length) {
        command->event->message = wb_string::ToUtf8String(message->message);
    }
    if (message->chatroom_talker_wxid && message->chatroom_talker_wxid_length) {
        command->event->chatroomTalkerWxid = wb_string::ToUtf8String(message->chatroom_talker_wxid);
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
    command->event->wxid        = wb_string::ToUtf8String(wxid->str);
    command->event->message     = wb_string::ToUtf8String(message->str);

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

void wxbot::WxBot::PluginSendMessageHandler(const wb_plugin::PluginSendWeChatMessagePtr& sendMessageArgs)
{
    if (!sendMessageArgs) {
        return;
    }

    switch (TO_WECHAT_MESSASGE_TYPE(sendMessageArgs->messageType)) {
        case wb_wx::WeChatMessageType::PLAINTEXT: {
            if (!sendMessageArgs->chatroom) {
                sendMessageArgs->useWxNumber ? SendTextMessageToContactWithWxNumber(sendMessageArgs->wxnumber, sendMessageArgs->message)
                                             : SendTextMessageToContact(sendMessageArgs->wxid, sendMessageArgs->message);
            }
            else {
                SendTextMessageToChatroomWithNotifyList(sendMessageArgs->wxid, sendMessageArgs->notifyWxidLists, sendMessageArgs->message);
            }
            break;
        }

        case wb_wx::WeChatMessageType::PICTURE: {
            sendMessageArgs->useWxNumber ? SendPictureToContactWithWxNumber(sendMessageArgs->wxnumber, sendMessageArgs->imgPath)
                                         : SendPictureToContact(sendMessageArgs->wxid, sendMessageArgs->imgPath);
            break;
        }

        case wb_wx::WeChatMessageType::FILE: {
            sendMessageArgs->useWxNumber ? SendFileToContactWithWxNumber(sendMessageArgs->wxnumber, sendMessageArgs->filePath)
                                         : SendFileToContact(sendMessageArgs->wxid, sendMessageArgs->filePath);
            break;
        }
    }
}

void wxbot::WxBot::PluginToHostEventHandler(const wb_plugin::PluginVirtualMachinePluginToHostEventPtr& pluginToHostEvent)
{
    if (!pluginToHostEvent || !pluginToHostEvent->hostEvent) {
        return;
    }

    auto event = pluginToHostEvent->hostEvent;

    switch (event->type) {
        case wb_plugin::HostEventType::SendMesage: {
            if (!event->sendMessageArgs) {
                break;
            }

            PluginSendMessageHandler(event->sendMessageArgs);
            break;
        }

        case wb_plugin::HostEventType::Log: {
            if (!event->log) {
                break;
            }

            this->log((wxbox::WxBotLogLevel)event->log->level, event->log->message.c_str());
            break;
        }

        case wb_plugin::HostEventType::ClearCommandResultScreen: {
            wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
            msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::CLEAR_COMMAND_LOG_REQUEST);
            client->PushMessageAsync(std::move(msg));
            break;
        }

        case wb_plugin::HostEventType::Logout: {
            if (wb_crack::IsLoign()) {
                wb_crack::Logout();
            }
            break;
        }

        case wb_plugin::HostEventType::ChangeConfig: {
            if (!event->changeConfig) {
                break;
            }

            if (!event->changeConfig->configName.compare("avoid_revoke")) {
                args->avoidRevokeMessage = event->changeConfig->enabled;
            }
            else if (!event->changeConfig->configName.compare("enable_raw_message_hook")) {
                args->enableRawMessageHook = event->changeConfig->enabled;
            }
            else if (!event->changeConfig->configName.compare("enable_send_text_message_hook")) {
                args->enableSendTextMessageHook = event->changeConfig->enabled;
            }

            break;
        }

        case wb_plugin::HostEventType::UnInject: {
            Stop();
            break;
        }

        case wb_plugin::HostEventType::ExitWxBox: {
            wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotResponse);
            msg.u.wxBotControlPacket.set_type(wxbox::ControlPacketType::EXIT_WXBOX_REQUEST);
            client->PushMessageAsync(std::move(msg));
            break;
        }

        case wb_plugin::HostEventType::ReportHelp: {
            std::string helpTxtFilePath = wb_file::JoinPath(args->plugins_root, "wxbox_apis.txt");
            if (wb_file::IsPathExists(wb_string::Utf8ToNativeString(helpTxtFilePath))) {
                SendFileToFileHelper(helpTxtFilePath);
            }
            break;
        }
    }
}