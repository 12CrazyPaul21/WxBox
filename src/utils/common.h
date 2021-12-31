#ifndef __WXBOX_UTILS_COMMON_H
#define __WXBOX_UTILS_COMMON_H

#include "config.h"

//
// Platform related header
//

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <Psapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")

#elif WXBOX_IN_MAC_OS

#include <unistd.h>
#include <sys/stat.h>

#endif


//
// C/C++ header
//

#include <experimental/filesystem>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <regex>
#include <cstdlib>


//
// third party
//

#include <frida-gum.h>
#include <TitanEngine.h>
#include <yaml-cpp/yaml.h>


//
// WxBox utils header
//

#include <utils/process.h>
#include <utils/wx.h>
#include <utils/file.h>
#include <utils/string.h>
#include <utils/memory.h>
#include <utils/feature.h>
#include <utils/config.hpp>


//
// Short namespace
//

namespace wb_process = wxbox::util::process;
namespace wb_wx      = wxbox::util::wx;
namespace wb_file    = wxbox::util::file;
namespace wb_string  = wxbox::util::string;
namespace wb_memory  = wxbox::util::memory;
namespace wb_feature = wxbox::util::feature;

#endif // #ifndef __WXBOX_UTILS_COMMON_H