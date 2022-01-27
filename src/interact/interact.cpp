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

                if (pPluginToHost->hostEvent->type == wb_plugin::HostEventType::SendTextMesage) {
                    std::cout << "host event type : <SendTextMesage>" << std::endl;
                    std::cout << "    send to : " << pPluginToHost->hostEvent->wxid << std::endl;
                    std::cout << "    text message : " << pPluginToHost->hostEvent->textMessage << std::endl;

                    auto command         = wb_plugin::BuildPluginVirtualMachineCommand<wb_plugin::PluginVirtualMachineCommandType::SendWxChatTextMessage>();
                    command->wxid        = pPluginToHost->hostEvent->wxid;
                    command->textMessage = pPluginToHost->hostEvent->textMessage;
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
