#include "config.h"
#include <wx.h>

#if WXBOX_IN_WINDOWS_OS

#include <Windows.h>
#include <memory>

#define WX_INSTALLATION_PATH_REGISTER_SUB_KEY "SOFTWARE\\Tencent\\WeChat"
#define WX_INSTALLATION_PATH_REGISTER_VALUE_NAME "InstallPath"

static inline std::string GetWxInstallationPath_Windows()
{
    LSTATUS status = ERROR_SUCCESS;
    DWORD cbData = 0;

	// calc installation path string buffer bytes count
    status = RegGetValueA(HKEY_CURRENT_USER,
                          WX_INSTALLATION_PATH_REGISTER_SUB_KEY,
                          WX_INSTALLATION_PATH_REGISTER_VALUE_NAME,
                          RRF_RT_REG_SZ,
                          nullptr,
                          nullptr,
                          &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    std::unique_ptr<char[]> tmp(new char[cbData]);
    status = RegGetValueA(HKEY_CURRENT_USER,
                          WX_INSTALLATION_PATH_REGISTER_SUB_KEY,
                          WX_INSTALLATION_PATH_REGISTER_VALUE_NAME,
                          RRF_RT_REG_SZ,
                          nullptr,
                          tmp.get(),
                          &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    return std::move(std::string(tmp.get()));
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetWxInstallationPath_Mac()
{
    return "";
}

#endif

std::string wxbox::util::GetWxInstallationPath()
{
    std::string wxInstalllationPath = "";

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    wxInstalllationPath = GetWxInstallationPath_Windows();
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    wxInstalllationPath = GetWxInstallationPath_Mac();
#endif

    return wxInstalllationPath;
}