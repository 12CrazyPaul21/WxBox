#ifndef __WXBOX_UTILS_COMMON_H
#define __WXBOX_UTILS_COMMON_H

#include "config.h"

//
// Platform related header
//

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#elif WXBOX_IN_MAC_OS

#include <unistd.h>
#include <sys/stat.h>

#endif

//
// C/C++ header
//

#include <experimental/filesystem>
#include <memory>

//
// WxBox utils header
//

#include <utils/process.h>
#include <utils/wx.h>
#include <utils/file.h>
#include <utils/config.hpp>

#endif // #ifndef __WXBOX_UTILS_COMMON_H