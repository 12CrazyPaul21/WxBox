#include <iostream>
#include <plugin/plugin.h>

using namespace std;

int main(int argc, char* argv[])
{
    WXBOX_UNREF(argc);
    WXBOX_UNREF(argv);

    auto pluginPath = wb_file::JoinPath(wb_file::GetProcessRootPath(), "/plugins");
    if (!wb_file::IsPathExists(pluginPath)) {
        pluginPath = wb_file::JoinPath(wb_file::GetProcessRootPath(), "../../../../plugins");
    }
    cout << "plugins path : " << pluginPath << std::endl;

    wb_plugin::PluginVirtualMachineStartupInfo startupInfo;
    startupInfo.pluginPath      = pluginPath;
    startupInfo.longTaskTimeout = WXBOX_PLUGIN_LONG_TASK_DEFAULT_TIMEOUT_MS;
    startupInfo.callback        = [](wb_plugin::PluginVirtualMachineEventPtr event) {
        switch (event->type) {
            case wb_plugin::PluginVirtualMachineEventType::ExecuteResult: {
                auto pExecuteResult = wb_plugin::CastPluginVirtualMachineEventPtr<wb_plugin::PluginVirtualMachineEventType::ExecuteResult>(event);
                if (!pExecuteResult) {
                    break;
                }

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
                if (!pPluginToHost || !pPluginToHost->hostEvent) {
                    break;
                }

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

                break;
            }
        }
    };

    // run virtual plugin machine
    if (!wb_plugin::StartPluginVirtualMachine(&startupInfo)) {
        std::cerr << "start plugin virtual machine failed" << std::endl;
        return 0;
    }

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
    return 0;
}
