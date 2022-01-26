#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

//
// Macro
//

#define WX_INSTALLATION_PATH_REGISTER_SUB_KEY "SOFTWARE\\Tencent\\WeChat"
#define WX_INSTALLATION_PATH_REGISTER_VALUE_NAME "InstallPath"

//
// Function
//

static inline std::string GetWxInstallationPath_Windows()
{
    return std::move(wb_platform::GetStringValueInRegister(HKEY_CURRENT_USER, WX_INSTALLATION_PATH_REGISTER_SUB_KEY, WX_INSTALLATION_PATH_REGISTER_VALUE_NAME));
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetWxInstallationPath_Mac()
{
    throw std::exception("GetWxInstallationPath_Mac stub");
    return "";
}

#endif

std::string wxbox::util::wx::GetWxInstallationPath()
{
    std::string wxInstalllationPath = "";

#if WXBOX_IN_WINDOWS_OS
    wxInstalllationPath = GetWxInstallationPath_Windows();
#elif WXBOX_IN_MAC_OS
    wxInstalllationPath = GetWxInstallationPath_Mac();
#endif

    return wxInstalllationPath;
}

bool wxbox::util::wx::IsWxInstallationPathValid(const std::string& installPath)
{
    if (installPath.length() == 0 || !wb_file::IsPathExists(installPath)) {
        return false;
    }

    std::vector<std::string> files = {
#if WXBOX_IN_WINDOWS_OS
        WX_WE_CHAT_EXE,
        WX_WE_CHAT_CORE_MODULE
#elif WXBOX_IN_MAC_OS
    // stub
#endif
    };

    for (auto file : files) {
        if (!wb_file::IsPathExists(wb_file::JoinPath(installPath, file))) {
            return false;
        }
    }

    return true;
}

std::string wxbox::util::wx::GetWxVersion(const std::string& installPath)
{
#if WXBOX_IN_WINDOWS_OS
    // get WeChat version info from WeChatWin.dll
    std::string weChatWinPath = wb_file::JoinPath(installPath, WX_WE_CHAT_CORE_MODULE);
    return wb_file::GetFileVersion(weChatWinPath);
#elif WXBOX_IN_MAC_OS
    throw std::exception("GetWxVersion stub");
    return "";
#endif
}

bool wxbox::util::wx::ResolveWxEnvInfo(const std::string& installPath, PWeChatEnvironmentInfo pWxEnvInfo)
{
    if (!pWxEnvInfo || !wb_wx::IsWxInstallationPathValid(installPath)) {
        return false;
    }

    pWxEnvInfo->installPath = installPath;

#if WXBOX_IN_WINDOWS_OS
    pWxEnvInfo->executeFileName = WX_WE_CHAT_EXE;
#elif WXBOX_IN_MAC_OS
    pWxEnvInfo->executeFileName = "Unknown";
#endif

    pWxEnvInfo->executeAbsPath = wb_file::JoinPath(installPath, pWxEnvInfo->executeFileName);
    pWxEnvInfo->version        = wb_wx::GetWxVersion(installPath);

    return true;
}

std::vector<wb_process::ProcessInfo> wxbox::util::wx::GetWeChatProcessList()
{
    auto processLists = wxbox::util::process::GetProcessList();

    processLists.erase(std::remove_if(processLists.begin(), processLists.end(), [&](const wb_process::ProcessInfo& pi) {
                           return ::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
                       }),
                       processLists.end());

    return std::move(processLists);
}

bool wxbox::util::wx::CheckWeChatProcessValid(wxbox::util::process::PID pid)
{
    wb_process::ProcessInfo pi;
    if (!wb_process::GetProcessInfoByPID(pid, pi)) {
        return false;
    }

    return !::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
}