#include <test_common.h>

//
// wxbox_plugin
//

TEST(wxbox_plugin, command_statement_parse)
{
#ifdef WXBOX_IN_WINDOWS_OS
    ::SetConsoleOutputCP(936);
#endif

    //
    // Legal statement
    //

    auto statement = ">>wxbox.foo: \"is a valid calling\", 2333, 1024, 727, false, \"is a string\", true, \"\u8fd9\u662f\u4e00\u4e2a\u4e2d\u6587\u5b57\u7b26\u4e32\"";
    spdlog::info("statement : {}", statement);
    auto parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::ModuleMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->moduleName.compare("wxbox"));
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("foo"));
    EXPECT_EQ(std::size_t(8), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = ">>wxbox.foo: \"is a valid calling\", 233, \"\'a\' is char\", \'abc\"hello\"cba, \"done\'";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::ModuleMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->moduleName.compare("wxbox"));
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("foo"));
    EXPECT_EQ(std::size_t(4), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  wxbox.foo";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::ModuleMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->moduleName.compare("wxbox"));
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  wxbox.foo:";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::ModuleMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->moduleName.compare("wxbox"));
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  wxbox.foo  : ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::ModuleMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->moduleName.compare("wxbox"));
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = ">>global_foo: \"is a valid calling\", 2333, 1024, 727, false, \"is a string\", true";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::GlobalMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("global_foo"));
    EXPECT_EQ(std::size_t(7), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  global_foo: \"is a valid calling\"";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::GlobalMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("global_foo"));
    EXPECT_EQ(std::size_t(1), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  global_foo";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::GlobalMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("global_foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  global_foo:";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::GlobalMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("global_foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    statement = "  >>  global_foo  : ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_NE(nullptr, parseResult);
    EXPECT_EQ(wb_plugin::CommandParseStatus::Success, parseResult->status);
    EXPECT_NE(nullptr, parseResult->commandInfo);
    EXPECT_EQ(wb_plugin::CommandMethodType::GlobalMethod, parseResult->commandInfo->type);
    EXPECT_EQ(0, parseResult->commandInfo->methodName.compare("global_foo"));
    EXPECT_EQ(std::size_t(0), parseResult->commandInfo->argLists.size());
    PrintCommandInfoLog(parseResult->commandInfo);

    //
    // Illegal statement
    //

    statement = " wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::NotCommand, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " > wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::NotCommand, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " > >> wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::NotCommand, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " > > > wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::NotCommand, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> > wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::LexIllegal, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo : \"ref\", 2333, \"";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::LexIllegal, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo : \"ref\", 2333, 111222333444555";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::LexIllegal, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> >> wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::RepeatPrefixSeparator, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> 1wxbox.foo ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::MethodNameNotSpecified, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox. 1 ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::InvalidModuleRef, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo 1 ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::MissArgBeginSeparator, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox_foo 1 ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::MissArgBeginSeparator, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo : ref";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::IllegalParameterList, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo : ,233";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::IllegalParameterList, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);

    statement = " >> wxbox.foo : \"ref\", 2333, ";
    spdlog::info("statement : {}", statement);
    parseResult = wb_plugin::ParseCommandStatement(statement);
    EXPECT_EQ(wb_plugin::CommandParseStatus::IllegalParameterList, parseResult->status);
    PrintCommandParseFailedMessageLog(parseResult);
}

TEST(wxbox_plugin, DISABLED_command_statement_parse_interact)
{
    char statement[1024];

    std::cout << "command statement interactive parse test <input 'quit' to exit>" << std::endl;

    for (;;) {
        std::cout << " > ";
        std::cin.getline(statement, sizeof(statement));
        std::cin.clear();

        if (!strlen(statement)) {
            continue;
        }

        if (!strcmp(statement, "quit")) {
            break;
        }

        auto parseResult = wb_plugin::ParseCommandStatement(statement);
        EXPECT_NE(nullptr, parseResult);
        if (parseResult->status == wb_plugin::CommandParseStatus::Success) {
            PrintCommandInfo(parseResult->commandInfo);
        }
        else {
            PrintCommandParseFailedMessage(parseResult);
        }
    }
}

TEST(wxbox_plugin, DISABLED_plugin_eval_command)
{
    auto pluginPath = wb_file::JoinPath(wb_file::GetProcessRootPath(), "../../../plugins");
    spdlog::info("plugin path : {}", pluginPath);

    wb_plugin::PluginVirtualMachineStartupInfo startupInfo;
    startupInfo.pluginPath      = pluginPath;
    startupInfo.longTaskTimeout = WXBOX_PLUGIN_LONG_TASK_DEFAULT_TIMEOUT_MS;
    startupInfo.callback        = [](wb_plugin::PluginVirtualMachineEventPtr event) {
        switch (event->type) {
            case wb_plugin::PluginVirtualMachineEventType::ExecuteResult: {
                auto pExecuteResult = wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::ExecuteResult>(event);
                EXPECT_NE(nullptr, pExecuteResult);

                if (!pExecuteResult->status) {
                    std::cout << pExecuteResult->error << std::endl;
                    break;
                }

                if (!pExecuteResult->results.size()) {
                    break;
                }

                //
                // print results
                //

                std::cout << "command initiator : " << (pExecuteResult->fromFilehelper ? "filehelper" : "wxbox") << std::endl;
                std::cout << "results : " << std::endl;

                for (auto result : pExecuteResult->results) {
                    switch (result->type) {
                        case wb_plugin::CommandExecuteResultType::Number:
                            std::cout << "    type : number, value : " << result->numeralval << std::endl;
                            break;
                        case wb_plugin::CommandExecuteResultType::String:
                            std::cout << "    type : string, value : " << result->strval << std::endl;
                            break;
                        case wb_plugin::CommandExecuteResultType::Boolean:
                            std::cout << "    type : boolean, value : " << std::boolalpha << result->boolval << std::endl;
                            break;
                    }
                }

                break;
            }
        }
    };

    // run virtual plugin machine
    EXPECT_EQ(true, wb_plugin::StartPluginVirtualMachine(&startupInfo));

    std::cout << "command statement interactive eval test <input 'quit' to exit>" << std::endl;

    char statement[1024];
    for (;;) {
        std::cout << " >> ";
        std::cin.getline(statement, sizeof(statement));
        std::cin.clear();

        if (!strlen(statement)) {
            continue;
        }

        if (!strcmp(statement, "quit")) {
            break;
        }

        auto command     = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::Eval>();
        command->command = ">>";
        command->command += statement;
        wb_plugin::PushPluginVirtualMachineCommand(command);
        command->signal.get_future().wait();
    }

    // stop virtual plugin machine
    wb_plugin::StopPluginVirtualMachine();
}

TEST(wxbox_plugin, text_message_handler)
{
    auto pluginPath = wb_file::JoinPath(wb_file::GetProcessRootPath(), "../../../plugins");
    spdlog::info("plugin path : {}", pluginPath);

    wb_plugin::PluginVirtualMachineStartupInfo startupInfo;
    startupInfo.pluginPath      = pluginPath;
    startupInfo.longTaskTimeout = WXBOX_PLUGIN_LONG_TASK_DEFAULT_TIMEOUT_MS;
    startupInfo.callback        = [](wb_plugin::PluginVirtualMachineEventPtr event) {
        switch (event->type) {
            case wb_plugin::PluginVirtualMachineEventType::ExecuteResult: {
                auto pExecuteResult = wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::ExecuteResult>(event);
                EXPECT_NE(nullptr, pExecuteResult);

                if (!pExecuteResult->status) {
                    std::cout << pExecuteResult->error << std::endl;
                    break;
                }

                if (!pExecuteResult->results.size()) {
                    break;
                }

                //
                // print results
                //

                std::cout << "command initiator : " << (pExecuteResult->fromFilehelper ? "filehelper" : "wxbox") << std::endl;
                std::cout << "results : " << std::endl;

                for (auto result : pExecuteResult->results) {
                    switch (result->type) {
                        case wb_plugin::CommandExecuteResultType::Number:
                            std::cout << "    type : number, value : " << result->numeralval << std::endl;
                            break;
                        case wb_plugin::CommandExecuteResultType::String:
                            std::cout << "    type : string, value : " << result->strval << std::endl;
                            break;
                        case wb_plugin::CommandExecuteResultType::Boolean:
                            std::cout << "    type : boolean, value : " << std::boolalpha << result->boolval << std::endl;
                            break;
                    }
                }

                break;
            }
            case wb_plugin::PluginVirtualMachineEventType::PluginToHost: {
                std::cout << "host event from plugin" << std::endl;
                auto pPluginToHost = wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::PluginToHost>(event);
                EXPECT_NE(nullptr, pPluginToHost);
                EXPECT_NE(nullptr, pPluginToHost->hostEvent);

                if (pPluginToHost->hostEvent->type == wb_plugin::HostEventType::SendMesage) {
                    auto sendMessageArgs = pPluginToHost->hostEvent->sendMessageArgs;
                    if (!sendMessageArgs) {
                        break;
                    }

                    std::cout << "host event type : <SendMesage>" << std::endl;
                    std::cout << "    message type : " << sendMessageArgs->messageType << std::endl;
                    std::cout << "    is chatroom : " << sendMessageArgs->chatroom << std::endl;
                    std::cout << "    use wxnumber : " << sendMessageArgs->useWxNumber << std::endl;
                    std::cout << "    wxid : " << sendMessageArgs->wxid << std::endl;
                    std::cout << "    wxnumber : " << sendMessageArgs->wxnumber << std::endl;
                    std::cout << "    message : " << sendMessageArgs->message << std::endl;
                    std::cout << "    image file path : " << sendMessageArgs->imgPath << std::endl;
                    std::cout << "    file path : " << sendMessageArgs->filePath << std::endl;
                    std::cout << "    notify list : " << std::endl;
                    for (auto notify : sendMessageArgs->notifyWxidLists) {
                        std::cout << "        " << notify << std::endl;
                    }

                    auto command   = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
                    command->event = wb_plugin::BuildPluginEventModel();
                    if (!command->event) {
                        return;
                    }

                    command->event->type               = wb_plugin::PluginEventType::ReceiveTextMessage;
                    command->event->pData1             = nullptr;
                    command->event->messageType        = (uint32_t)wb_wx::WeChatMessageType::PLAINTEXT;
                    command->event->wxid               = "fake_wxid";
                    command->event->message            = "message";
                    command->event->chatroomTalkerWxid = "im";

                    wb_plugin::PushPluginVirtualMachineCommand(command);
                    command->signal.get_future().wait();
                }
            }
        }
    };

    // run virtual plugin machine
    EXPECT_EQ(true, wb_plugin::StartPluginVirtualMachine(&startupInfo));

    //
    // test case
    //

    auto recvcommand            = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    recvcommand->event          = wb_plugin::BuildPluginEventModel();
    recvcommand->event->type    = wb_plugin::PluginEventType::ReceiveTextMessage;
    recvcommand->event->wxid    = "filehelper";
    recvcommand->event->message = ">>wxbox.version";
    wb_plugin::PushPluginVirtualMachineCommand(recvcommand);
    recvcommand->signal.get_future().wait();

    recvcommand                 = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    recvcommand->event          = wb_plugin::BuildPluginEventModel();
    recvcommand->event->type    = wb_plugin::PluginEventType::ReceiveTextMessage;
    recvcommand->event->wxid    = "hape";
    recvcommand->event->message = ">>wxbox.version";
    wb_plugin::PushPluginVirtualMachineCommand(recvcommand);
    recvcommand->signal.get_future().wait();

    auto sendcommand            = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    sendcommand->event          = wb_plugin::BuildPluginEventModel();
    sendcommand->event->type    = wb_plugin::PluginEventType::SendTextMessage;
    sendcommand->event->wxid    = "filehelper";
    sendcommand->event->message = ">>wxbox.version";
    wb_plugin::PushPluginVirtualMachineCommand(sendcommand);
    sendcommand->signal.get_future().wait();

    sendcommand                 = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::WeChatLifeEventMessage>();
    sendcommand->event          = wb_plugin::BuildPluginEventModel();
    sendcommand->event->type    = wb_plugin::PluginEventType::SendTextMessage;
    sendcommand->event->wxid    = "hape";
    sendcommand->event->message = "hello wxbox plugin";
    wb_plugin::PushPluginVirtualMachineCommand(sendcommand);
    sendcommand->signal.get_future().wait();

    // stop virtual plugin machine
    wb_plugin::StopPluginVirtualMachine();
}