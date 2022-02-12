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
    return wb_platform::GetStringValueInRegister(HKEY_CURRENT_USER, WX_INSTALLATION_PATH_REGISTER_SUB_KEY, WX_INSTALLATION_PATH_REGISTER_VALUE_NAME);
}

static inline std::string GetWxModuleFolderPath_Windows(const std::string& installPath)
{
    if (installPath.empty() || !wb_file::IsPathExists(installPath)) {
        return "";
    }

    if (wb_file::IsPathExists(wb_file::JoinPath(installPath, WX_WE_CHAT_CORE_MODULE))) {
        return installPath;
    }

    std::vector<std::string> subdirs = wb_file::ListFolderInDirectory(installPath);
    if (subdirs.empty()) {
        return "";
    }

    std::tuple<wb_file::VersionNumber, std::string> newerModuleInfo;
    for (auto subdir : subdirs) {
        std::string absFolderPath = wb_file::JoinPath(installPath, subdir);
        std::string modulePath    = wb_file::JoinPath(absFolderPath, WX_WE_CHAT_CORE_MODULE);
        if (!wb_file::IsPathExists(modulePath)) {
            continue;
        }

        wb_file::VersionNumber versionNumber;
        if (!wb_file::UnwindVersionNumber(wb_file::GetFileVersion(modulePath), versionNumber)) {
            continue;
        }

        if (std::get<1>(newerModuleInfo).empty() || versionNumber >= std::get<0>(newerModuleInfo)) {
            std::get<0>(newerModuleInfo) = std::move(versionNumber);
            std::get<1>(newerModuleInfo) = std::move(absFolderPath);
            continue;
        }
    }

    return std::get<1>(newerModuleInfo);
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetWxInstallationPath_Mac()
{
    throw std::exception("GetWxInstallationPath_Mac stub");
    return "";
}

static inline std::string GetWxModuleFolderPath_Mac(const std::string& installPath)
{
    throw std::exception("GetWxModuleFolderPath_Mac stub");
    return "";
}

#endif

std::string wxbox::util::wx::GetWxInstallationPath()
{
#if WXBOX_IN_WINDOWS_OS
    return GetWxInstallationPath_Windows();
#elif WXBOX_IN_MAC_OS
    return GetWxInstallationPath_Mac();
#endif
}

std::string wxbox::util::wx::GetWxModuleFolderPath(const std::string& installPath)
{
#if WXBOX_IN_WINDOWS_OS
    return GetWxModuleFolderPath_Windows(installPath);
#elif WXBOX_IN_MAC_OS
    return GetWxModuleFolderPath_Mac(installPath);
#endif
}

std::string wxbox::util::wx::GetWxVersion(const std::string& moduleFolderPath)
{
#if WXBOX_IN_WINDOWS_OS
    // get WeChat version info from WeChatWin.dll
    std::string weChatWinPath = wb_file::JoinPath(moduleFolderPath, WX_WE_CHAT_CORE_MODULE);
    return wb_file::GetFileVersion(weChatWinPath);
#elif WXBOX_IN_MAC_OS
    throw std::exception("GetWxVersion stub");
    return "";
#endif
}

bool wxbox::util::wx::IsWxInstallationPathValid(const std::string& installPath)
{
    if (installPath.length() == 0 || !wb_file::IsPathExists(installPath)) {
        return false;
    }

    if (!wb_file::IsPathExists(wb_file::JoinPath(installPath, WX_WE_CHAT_EXE))) {
        return false;
    }

    if (GetWxModuleFolderPath(installPath).empty()) {
        return false;
    }

    return true;
}

bool wxbox::util::wx::IsWxInstallationPathValid(const std::string& installPath, const std::string& moduleFolderPath)
{
    if (installPath.empty() || !wb_file::IsPathExists(installPath) || moduleFolderPath.empty() || !wb_file::IsPathExists(moduleFolderPath)) {
        return false;
    }

    if (!wb_file::IsPathExists(wb_file::JoinPath(installPath, WX_WE_CHAT_EXE))) {
        return false;
    }

    return wb_file::IsPathExists(wb_file::JoinPath(moduleFolderPath, WX_WE_CHAT_CORE_MODULE));
}

bool wxbox::util::wx::ResolveWxEnvInfo(WeChatEnvironmentInfo& wxEnvInfo)
{
    auto wxProcessList = GetWeChatProcessList();
    if (!wxProcessList.empty() && ResolveWxEnvInfo(wxProcessList[0].dirpath, wxEnvInfo)) {
        return true;
    }

    std::string installPath = GetWxInstallationPath();
    if (installPath.empty()) {
        return false;
    }

    return ResolveWxEnvInfo(installPath, wxEnvInfo);
}

bool wxbox::util::wx::ResolveWxEnvInfo(const wb_process::PID& pid, WeChatEnvironmentInfo& wxEnvInfo)
{
    std::string installPath = wb_file::GetProcessRootPath(pid);
    if (installPath.empty()) {
        return false;
    }

    wb_process::ModuleInfo coreModuleInfo;
    if (!wb_process::GetModuleInfo(pid, WX_WE_CHAT_CORE_MODULE, coreModuleInfo)) {
        return false;
    }

    return ResolveWxEnvInfo(installPath, wb_file::ToDirectoryPath(coreModuleInfo.modulePath), wxEnvInfo);
}

bool wxbox::util::wx::ResolveWxEnvInfo(const std::string& installPath, WeChatEnvironmentInfo& wxEnvInfo)
{
    return ResolveWxEnvInfo(installPath, GetWxModuleFolderPath(installPath), wxEnvInfo);
}

bool wxbox::util::wx::ResolveWxEnvInfo(const std::string& installPath, const std::string& moduleFolderPath, WeChatEnvironmentInfo& wxEnvInfo)
{
    if (!wb_wx::IsWxInstallationPathValid(installPath, moduleFolderPath)) {
        return false;
    }

    wxEnvInfo.installPath = installPath;

#if WXBOX_IN_WINDOWS_OS
    wxEnvInfo.executeFileName = WX_WE_CHAT_EXE;
    wxEnvInfo.coreModuleName  = WX_WE_CHAT_CORE_MODULE;
#elif WXBOX_IN_MAC_OS
    wxEnvInfo.executeFileName = "Unknown";
    wxEnvInfo.coreModuleName  = "Unknown";
#endif

    wxEnvInfo.executeAbsPath      = wb_file::JoinPath(installPath, wxEnvInfo.executeFileName);
    wxEnvInfo.moduleFolderAbsPath = moduleFolderPath;
    wxEnvInfo.coreModuleAbsPath   = wb_file::JoinPath(moduleFolderPath, wxEnvInfo.coreModuleName);
    wxEnvInfo.version             = wb_wx::GetWxVersion(moduleFolderPath);

    return true;
}

std::vector<wb_process::ProcessInfo> wxbox::util::wx::GetWeChatProcessList()
{
    auto processLists = wxbox::util::process::GetProcessList();

    processLists.erase(std::remove_if(processLists.begin(), processLists.end(), [&](const wb_process::ProcessInfo& pi) {
                           return ::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
                       }),
                       processLists.end());

    return processLists;
}

bool wxbox::util::wx::CheckWeChatProcessValid(wxbox::util::process::PID pid)
{
    wb_process::ProcessInfo pi;
    if (!wb_process::GetProcessInfoByPID(pid, pi)) {
        return false;
    }

    return !::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
}