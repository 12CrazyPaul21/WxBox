#ifndef __WXBOX_TEST_COMMON_H
#define __WXBOX_TEST_COMMON_H

#include "config.h"

#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <utils/common.h>
#include <plugin/plugin.h>

#include <wxbox/app_config.hpp>
#include <wxbox/app_log.hpp>

#ifdef WXBOX_IN_WINDOWS_OS
#include <conio.h>
#else

#ifndef _getch
char _getch()
{
    char retval = 0;

    system("stty -echo");
    system("stty -icanon");
    retval = getchar();
    system("stty icanon");
    system("stty echo");

    return retval;
}
#endif

#endif

#define WaitForPressAnyKey(msg)        \
    {                                  \
        std::cout << msg << std::endl; \
        _getch();                      \
    }

#define PrintUInt8Vector(vtUInt8)                                                                                          \
    {                                                                                                                      \
        std::cout << "[ ";                                                                                                 \
        std::for_each(vtUInt8.begin(), vtUInt8.end(), [&](auto byte) {                                                     \
            std::cout << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << uint16_t(byte) << " "; \
        });                                                                                                                \
        std::cout << "]" << std::endl;                                                                                     \
    }

#define PrintCommandInfoLog(commandInfo)                                                                  \
    {                                                                                                     \
        switch (commandInfo->type) {                                                                      \
            case wb_plugin::CommandMethodType::ModuleMethod:                                              \
                spdlog::info("calling method : {}.{}", commandInfo->moduleName, commandInfo->methodName); \
                break;                                                                                    \
            case wb_plugin::CommandMethodType::GlobalMethod:                                              \
                spdlog::info("calling method : {}", commandInfo->methodName);                             \
                break;                                                                                    \
        }                                                                                                 \
                                                                                                          \
        spdlog::info("    parameters({}) : ", commandInfo->argLists.size());                              \
        for (auto& parameter : commandInfo->argLists) {                                                   \
            switch (parameter.type) {                                                                     \
                case wb_plugin::CommandParameterType::NumeralLiteral:                                     \
                    spdlog::info("        type : number, value : {}", parameter.numeralval);              \
                    break;                                                                                \
                case wb_plugin::CommandParameterType::StringLiteral:                                      \
                    spdlog::info("        type : string, value : {}", parameter.strval);                  \
                    break;                                                                                \
                case wb_plugin::CommandParameterType::BooleanLiteral:                                     \
                    spdlog::info("        type : boolean, value : {}", parameter.boolval);                \
                    break;                                                                                \
            }                                                                                             \
        }                                                                                                 \
    }

#define PrintCommandParseFailedMessageLog(parseResult)                          \
    {                                                                           \
        spdlog::info("command parse error message : [{}]", parseResult->error); \
    }

#define PrintCommandInfo(commandInfo)                                                                                       \
    {                                                                                                                       \
        switch (commandInfo->type) {                                                                                        \
            case wb_plugin::CommandMethodType::ModuleMethod:                                                                \
                std::cout << "calling method : " << commandInfo->moduleName << "." << commandInfo->methodName << std::endl; \
                break;                                                                                                      \
            case wb_plugin::CommandMethodType::GlobalMethod:                                                                \
                std::cout << "calling method : " << commandInfo->methodName << std::endl;                                   \
                break;                                                                                                      \
        }                                                                                                                   \
                                                                                                                            \
        std::cout << "    parameters(" << commandInfo->argLists.size() << ") :" << std::endl;                               \
        for (auto& parameter : commandInfo->argLists) {                                                                     \
            switch (parameter.type) {                                                                                       \
                case wb_plugin::CommandParameterType::NumeralLiteral:                                                       \
                    std::cout << "        type : number, value : " << parameter.numeralval << std::endl;                    \
                    break;                                                                                                  \
                case wb_plugin::CommandParameterType::StringLiteral:                                                        \
                    std::cout << "        type : string, value : " << parameter.strval << std::endl;                        \
                    break;                                                                                                  \
                case wb_plugin::CommandParameterType::BooleanLiteral:                                                       \
                    std::cout << "        type : boolean, value : " << parameter.boolval << std::endl;                      \
                    break;                                                                                                  \
            }                                                                                                               \
        }                                                                                                                   \
    }

#define PrintCommandParseFailedMessage(parseResult)                                               \
    {                                                                                             \
        std::cout << "command parse error message : [" << parseResult->error << "]" << std::endl; \
    }

#endif  // #ifndef __WXBOX_TEST_COMMON_H