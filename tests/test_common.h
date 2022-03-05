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

#define PrintWxApiFeatures(FEATURES_INFO)                                                          \
    {                                                                                              \
        spdlog::info("WxApiFeatures platform : {}", FEATURES_INFO.platform);                       \
        spdlog::info("WxApiFeatures featureFileAbsPath : {}", FEATURES_INFO.featureFolderAbsPath); \
        for (auto v : FEATURES_INFO.featureVersionList) {                                          \
            spdlog::info("    : {}", v.str);                                                       \
        }                                                                                          \
    }

#define PrintWxAbsoluteHookInfo(FEATURES_INFO)                                                 \
    {                                                                                          \
        for (auto absoluteHookInfoPair : FEATURES_INFO.mapWxAbsoluteHookInfo) {                \
            std::string                    wxVersion          = absoluteHookInfoPair.first;    \
            wb_feature::WxAbsoluteHookInfo wxAbsoluteHookInfo = absoluteHookInfoPair.second;   \
                                                                                               \
            spdlog::info("WxApiHookInfo absoluteHookInfo wechat version : {}", wxVersion);     \
            for (auto api : wb_feature::WX_HOOK_API) {                                         \
                spdlog::info("    {} RVA : 0x{:08X}", api, wxAbsoluteHookInfo.GetApiRva(api)); \
            }                                                                                  \
        }                                                                                      \
    }

#define PrintWxHookPointFeatures(FEATURES_INFO)                                                                      \
    {                                                                                                                \
        for (auto wxHookPointFeaturesPair : FEATURES_INFO.mapWxHookPointFeatures) {                                  \
            std::string                      wxVersion         = wxHookPointFeaturesPair.first;                      \
            wb_feature::WxHookPointFeatures  hookPointFeatures = wxHookPointFeaturesPair.second;                     \
            wb_feature::HookPointFeatureInfo hookFeatureInfo;                                                        \
                                                                                                                     \
            spdlog::info("WxApiHookInfo hookPointFeatures wechat version : {}", wxVersion);                          \
            for (auto api : wb_feature::WX_HOOK_API) {                                                               \
                spdlog::info("    {} hook feature :", api);                                                          \
                if (!hookPointFeatures.GetApiHookFeature(api, hookFeatureInfo)) {                                    \
                    continue;                                                                                        \
                }                                                                                                    \
                                                                                                                     \
                spdlog::info("        ScanType : {}", hookFeatureInfo.scanType);                                     \
                if (!hookFeatureInfo.scanType.compare("ref")) {                                                      \
                    if (hookFeatureInfo.refFeatureStream.size()) {                                                   \
                        spdlog::info("        RefFeatureStream :");                                                  \
                        PrintUInt8Vector(hookFeatureInfo.refFeatureStream);                                          \
                    }                                                                                                \
                                                                                                                     \
                    if (hookFeatureInfo.refBackExtralInstruction.size()) {                                           \
                        spdlog::info("        RefBackExtralInstruction :");                                          \
                        PrintUInt8Vector(hookFeatureInfo.refBackExtralInstruction);                                  \
                    }                                                                                                \
                                                                                                                     \
                    if (hookFeatureInfo.refFrontExtralInstruction.size()) {                                          \
                        spdlog::info("        RefFrontExtralInstruction :");                                         \
                        PrintUInt8Vector(hookFeatureInfo.refFrontExtralInstruction);                                 \
                    }                                                                                                \
                }                                                                                                    \
                else if (!hookFeatureInfo.scanType.compare("multiPushRef")) {                                        \
                    if (hookFeatureInfo.pushInstruction.size()) {                                                    \
                        spdlog::info("        PushInstruction :");                                                   \
                        PrintUInt8Vector(hookFeatureInfo.pushInstruction);                                           \
                    }                                                                                                \
                                                                                                                     \
                    if (hookFeatureInfo.refFeatureStreams.size()) {                                                  \
                        spdlog::info("        RefFeatureStreams :");                                                 \
                        for (auto refFeatureStream : hookFeatureInfo.refFeatureStreams) {                            \
                            PrintUInt8Vector(refFeatureStream);                                                      \
                        }                                                                                            \
                    }                                                                                                \
                }                                                                                                    \
                else if (!hookFeatureInfo.scanType.compare("instruction")) {                                         \
                    if (hookFeatureInfo.instructionFeatureStream.size()) {                                           \
                        spdlog::info("        InstructionFeatureStream :");                                          \
                        PrintUInt8Vector(hookFeatureInfo.instructionFeatureStream);                                  \
                    }                                                                                                \
                }                                                                                                    \
                                                                                                                     \
                spdlog::info("        LocateAction : {}", hookFeatureInfo.locateAction);                             \
                if (hookFeatureInfo.locateActionFeatureStream.size()) {                                              \
                    spdlog::info("        LocateActionFeatureStream :");                                             \
                    PrintUInt8Vector(hookFeatureInfo.locateActionFeatureStream);                                     \
                }                                                                                                    \
                spdlog::info("        HookPointOffset : {}", hookFeatureInfo.hookPointOffset);                       \
                if (!hookFeatureInfo.locateAction.compare("backThenFront")) {                                        \
                    if (hookFeatureInfo.thenLocateActionFeatureStream.size()) {                                      \
                        spdlog::info("        ThenLocateActionFeatureStream :");                                     \
                        PrintUInt8Vector(hookFeatureInfo.thenLocateActionFeatureStream);                             \
                    }                                                                                                \
                }                                                                                                    \
                else if (!hookFeatureInfo.locateAction.compare("backMultiTimes")) {                                  \
                    spdlog::info("        LocateActionExecuteTimes : {}", hookFeatureInfo.locateActionExecuteTimes); \
                }                                                                                                    \
            }                                                                                                        \
        }                                                                                                            \
    }

#endif  // #ifndef __WXBOX_TEST_COMMON_H