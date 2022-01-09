#include <iostream>
#include <wxbot.hpp>
#include <spdlog/spdlog.h>

#include "config.h"

using namespace std;

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>

BOOL ConsoleCtrlHandler(DWORD ctrlType)
{
    UNREFERENCED_PARAMETER(ctrlType);

    TerminateProcess(GetCurrentProcess(), 0);
    return FALSE;
}

#else

#include <csignal>
#include <cstdlib>

void SignalHandler(int sig)
{
    exit(0);
}

#endif

void RegisterSignal()
{
#if WXBOX_IN_WINDOWS_OS
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);
#else
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGQUIT, SignalHandler);
#endif
}

void help()
{
    cout << "Usage:" << endl;
    cout << "[q] shutdown" << endl;
    cout << "[p] ping" << endl;
    cout << " > ";
}

void ping(wxbot::WxBot& bot)
{
    spdlog::info("Ping WxBoxServer...");
    if (!bot.Ping()) {
        spdlog::info("Ping WxBoxServer failed");
    }
    else {
        spdlog::info("WxBoxServer pong");
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    wxbot::WxBot bot;

    // handle quit/terminate signal
    RegisterSignal();

    //
    // step 1 : Regular Testing
    //

    spdlog::info("============================================");
    spdlog::info("          step 1 : Regular Testing");
    spdlog::info("============================================");

    spdlog::info("Create WxBoxClient");
    if (!bot.BuildWxBoxClient()) {
        spdlog::info("Create WxBoxClient failed");
        return 1;
    }

    spdlog::info("Start WxBoxClient");
    if (!bot.StartWxBoxClient()) {
        spdlog::info("Start WxBoxClient failed");
        return 1;
    }

    for (;;) {
        help();

        int command = cin.get();
        cin.ignore();

        if (command == 'q') {
            spdlog::info("Stop WxBoxClient");
            bot.StopWxBoxClient();
            break;
        }
        else if (command == 'p') {
            ping(bot);
        }
    }

    spdlog::info("Wait for WxBoxClient shutdown...");
    bot.Wait();

    spdlog::info("Destroy WxBoxClient");
    bot.DestroyWxBoxClient();

    spdlog::info("============================================");
    spdlog::info("          step 1 : Regular Testing [Finish]");
    spdlog::info("============================================");

    //
    // step 2 : Stress Testing
    //

    spdlog::info("============================================");
    std::cout << "          step 2 : Stress Testing" << endl;
    spdlog::info("============================================");

#define RECONSTRUCT_TIMES 1000
#define RESTART_TIMES 500

    for (int i = 0; i < RECONSTRUCT_TIMES; i++) {
        spdlog::info("Create WxBoxClient : {}", i);
        if (!bot.BuildWxBoxClient()) {
            spdlog::info("Create WxBoxClient failed : {}", i);
            return 1;
        }

        for (int j = 0; j < RESTART_TIMES; j++) {
            int times = i * RESTART_TIMES + j;

            spdlog::info("Start WxBoxClient : {}", times);
            if (!bot.StartWxBoxClient()) {
                spdlog::info("Start WxBoxClient failed : {}", i);
                return 1;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            spdlog::info("Stop WxBoxClient : {}", times);
            bot.StopWxBoxClient();

            spdlog::info("Wait for WxBoxClient shutdown : {}", times);
            bot.Wait();
        }

        spdlog::info("Destroy WxBoxClient : {}", i);
        bot.DestroyWxBoxClient();
    }

    spdlog::info("============================================");
    std::cout << "          step 2 : Stress Testing [Finish]" << endl;
    spdlog::info("============================================");

    bot.Shutdown();
    return 0;
}
