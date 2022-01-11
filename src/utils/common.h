#ifndef __WXBOX_UTILS_COMMON_H
#define __WXBOX_UTILS_COMMON_H

#include "config.h"

//
// Macro
//

#if defined(_MSC_VER)
#define PRAGMA __pragma
#else
#define PRAGMA _Pragma
#endif

#if WXBOX_IN_WINDOWS_OS
#define CloseHandleSafe(h)    \
    {                         \
        if (h) {              \
            ::CloseHandle(h); \
            h = NULL;         \
        }                     \
    }
#else
#define CloseHandleSafe(h) \
    {}
#endif

//
// Typedef
//

typedef long          cpulong_t;
typedef unsigned long ucpulong_t;

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

#include <utils/traits.h>
#include <utils/config.hpp>
#include <utils/process.h>
#include <utils/feature.h>
#include <utils/file.h>
#include <utils/memory.h>
#include <utils/string.h>
#include <utils/wx.h>
#include <utils/crack.h>
#include <utils/inject.h>

//
// Short namespace
//

namespace wb_traits  = wxbox::util::traits;
namespace wb_process = wxbox::util::process;
namespace wb_wx      = wxbox::util::wx;
namespace wb_file    = wxbox::util::file;
namespace wb_string  = wxbox::util::string;
namespace wb_memory  = wxbox::util::memory;
namespace wb_feature = wxbox::util::feature;
namespace wb_crack   = wxbox::util::crack;
namespace wb_inject  = wxbox::util::inject;

#endif  // #ifndef __WXBOX_UTILS_COMMON_H