#ifndef __WXBOX_UTILS_COMMON_H
#define __WXBOX_UTILS_COMMON_H

#include "config.h"

//
// Macro
//

#define U_OBJ_CONSTRUCTOR(TYPE, MEMBER, MEMBER_TYPE) \
    if (type == TYPE) {                              \
        new (&MEMBER) MEMBER_TYPE();                 \
        return;                                      \
    }

#define U_OBJ_DESTRUCTOR(TYPE, MEMBER, DESTRUCTOR) \
    if (type == TYPE) {                            \
        MEMBER.~DESTRUCTOR();                      \
        return;                                    \
    }

#define U_OBJ_COPY(TYPE, MEMBER, MEMBER_TYPE, OTHER) \
    if (type == TYPE) {                              \
        new (&MEMBER) MEMBER_TYPE(OTHER.MEMBER);     \
        return;                                      \
    }

#define U_OBJ_MOVE(TYPE, MEMBER, MEMBER_TYPE, OTHER)        \
    if (type == TYPE) {                                     \
        new (&MEMBER) MEMBER_TYPE(std::move(OTHER.MEMBER)); \
        return;                                             \
    }

#define U_SCALAR_CONSTRUCTOR(TYPE, MEMBER, DEFAULT_VALUE) \
    if (type == TYPE) {                                   \
        MEMBER = DEFAULT_VALUE;                           \
        return;                                           \
    }

#define U_SCALAR_COPY(TYPE, MEMBER, OTHER) \
    if (type == TYPE) {                    \
        MEMBER = OTHER.MEMBER;             \
        return;                            \
    }

#define SETUP_COPY_METHOD(TYPE, OTHER_VAL_NAME) \
    TYPE(const TYPE& OTHER_VAL_NAME)            \
    {                                           \
        __copy(OTHER_VAL_NAME);                 \
    }                                           \
    TYPE& operator=(const TYPE& OTHER_VAL_NAME) \
    {                                           \
        __copy(OTHER_VAL_NAME);                 \
        return *this;                           \
    }                                           \
                                                \
    void __copy(const TYPE& OTHER_VAL_NAME)

#define SETUP_MOVE_METHOD(TYPE, OTHER_VAL_NAME) \
    TYPE(TYPE&& OTHER_VAL_NAME)                 \
    {                                           \
        __move(std::move(OTHER_VAL_NAME));      \
    }                                           \
    TYPE& operator=(TYPE&& OTHER_VAL_NAME)      \
    {                                           \
        __move(std::move(OTHER_VAL_NAME));      \
        return *this;                           \
    }                                           \
                                                \
    void __move(TYPE&& OTHER_VAL_NAME)

//
// Typedef
//

typedef long     cpulong_t;
typedef unsigned ucpulong_t;

//
// Platform related headers
//

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <TlHelp32.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Version.lib")

#elif WXBOX_IN_MAC_OS

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

//
// C/C++ headers
//

#include <algorithm>
#include <cstdlib>
#include <experimental/filesystem>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

//
// Third party headers
//

#include <TitanEngine.h>
#include <frida-gum.h>
#include <yaml-cpp/yaml.h>

//
// Patch
//

#if !WXBOX_IN_WINDOWS_OS
#ifdef _stricmp
#define _stricmp strcmp
#endif
#endif

//
// WxBox utils headers
//

#include <utils/config.hpp>
#include <utils/process.h>
#include <utils/feature.h>
#include <utils/file.h>
#include <utils/memory.h>
#include <utils/string.h>
#include <utils/wx.h>
#include <utils/crack.h>

//
// Short namespace
//

namespace wb_process = wxbox::util::process;
namespace wb_wx      = wxbox::util::wx;
namespace wb_file    = wxbox::util::file;
namespace wb_string  = wxbox::util::string;
namespace wb_memory  = wxbox::util::memory;
namespace wb_feature = wxbox::util::feature;
namespace wb_crack   = wxbox::util::crack;

#endif  // #ifndef __WXBOX_UTILS_COMMON_H