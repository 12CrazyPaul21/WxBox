#include "plugin.h"

#include <Windows.h>
#include <vector>
#include <string>
#include <cctype>
#include <cwctype>
#include <sstream>

static inline std::wstring
toWString(const char* str)
{
    std::wstring wstr;

    if(!str || !*str) {
        return wstr;
    }

    int required = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if(required > 0) {
        wstr.resize(required - 1);
        if(!MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)wstr.c_str(), required)) {
            wstr.clear();
        }
    }

    return std::move(wstr);
}

static inline std::vector<std::string>
split(const std::string& str, const char& sep)
{
    std::vector<std::string> result;
    std::string::size_type pos1 = 0, pos2 = str.find(sep);

    while (pos2 != std::string::npos) {
        result.push_back(std::move(str.substr(pos1, pos2 - pos1)));
        pos1 = pos2 + 1;
        pos2 = str.find(sep, pos1);
    }

    if (pos1 != str.length()) {
        result.push_back(std::move(str.substr(pos1)));
    }

    return std::move(result);
}

static inline bool
xstrstr(const char* reg, const bool isUtf16, const bool isRef, const char* pattern)
{
    if (!reg || !pattern) {
        return false;
    }

    std::stringstream ss;
    ss << "strstr(utf" << (isUtf16 ? "16" : "8") << "("
        << (isRef ? "ReadDword(" : "") << reg
        << (isRef ? ")), \"" : "), \"") << pattern << "\")";

    bool success = false;
    duint result = DbgEval(ss.str().c_str(), &success);

    return success && result;
}

static bool xcontains(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
{
    duint matched = 0;
    std::string matchedReg = "";
    const char *pattern = argv[1].string.ptr;

#define MATCHED_PATTERN() { \
    matched = 1; \
    matchedReg = reg; \
}

    for (auto reg : split(argv[0].string.ptr, '|')) {

        // check memory valid
        duint address = DbgValFromString(reg.c_str());
        if (!DbgMemIsValidReadPtr(address)) {
            continue;
        }

        // utf8
        if (xstrstr(reg.c_str(), false, false, pattern)) {
            MATCHED_PATTERN();
            break;
        }

        // utf16
        if (xstrstr(reg.c_str(), true, false, pattern)) {
            MATCHED_PATTERN();
            break;
        }

        // ref utf8
        if (xstrstr(reg.c_str(), false, true, pattern)) {
            MATCHED_PATTERN();
            break;
        }

        // ref utf16
        if (xstrstr(reg.c_str(), true, true, pattern)) {
            MATCHED_PATTERN();
            break;
        }
    }

    if (matched) {
        dprintf("xcontains found \"%s\" in \"%s\" register\n", pattern, matchedReg.c_str());
    }

    result->type = ValueTypeNumber;
    result->number = matched;

    return true;
}

//Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    //
    // register "xcontains" expression function
    //

    ValueType argsType[] = {
        ValueTypeString,
        ValueTypeString
    };

    if (!_plugin_registerexprfunctionex(pluginHandle, "xcontains", ValueTypeNumber, argsType, 2, xcontains, nullptr)) {
        dputs("[" PLUGIN_NAME "] Error registering the \"xcontains\" expression function!");
    }

    return true; //Return false to cancel loading the plugin.
}

//Deinitialize your plugin data here.
void pluginStop()
{
}

//Do GUI/Menu related things here.
void pluginSetup()
{
}
