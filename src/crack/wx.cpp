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

typedef struct _RaiseWeChatWindowToForeground_EnumWindowProc_Parameter
{
    wb_process::PID pid;
    HWND&           hWndWeChat;
    bool            findMainWindow;
} RaiseWeChatWindowToForeground_EnumWindowProc_Parameter, *PRaiseWeChatWindowToForeground_EnumWindowProc_Parameter;

static BOOL CALLBACK RaiseWeChatWindowToForeground_EnumWindowProc(HWND hWnd, LPARAM lParam)
{
    PRaiseWeChatWindowToForeground_EnumWindowProc_Parameter parameter = reinterpret_cast<PRaiseWeChatWindowToForeground_EnumWindowProc_Parameter>(lParam);
    if (!parameter) {
        return FALSE;
    }

    DWORD dwPid = 0;
    GetWindowThreadProcessId(hWnd, &dwPid);
    if (dwPid != parameter->pid) {
        return TRUE;
    }

    char wndClassName[MAX_PATH] = {0};
    if (!GetClassNameA(hWnd, wndClassName, sizeof(wndClassName))) {
        return TRUE;
    }

    if (parameter->findMainWindow && !strcmp(wndClassName, WX_WE_CHAT_MAIN_WINDOW_CLASS_NAME)) {
        parameter->hWndWeChat = hWnd;
        return FALSE;
    }

    if (!parameter->findMainWindow && !strcmp(wndClassName, WX_WE_CHAT_LOGIN_WINDOW_CLASS_NAME)) {
        parameter->hWndWeChat = hWnd;
        return FALSE;
    }

    return TRUE;
}

static bool RaiseWeChatWindowToForeground_Windows(const wb_process::PID& pid)
{
    HWND                                                   hWnd = NULL;
    RaiseWeChatWindowToForeground_EnumWindowProc_Parameter parameter{pid, hWnd, true};

    // try to find main window
    ::EnumWindows(RaiseWeChatWindowToForeground_EnumWindowProc, (LPARAM)&parameter);
    if (!hWnd) {
        // try to find login window
        parameter.findMainWindow = false;
        ::EnumWindows(RaiseWeChatWindowToForeground_EnumWindowProc, (LPARAM)&parameter);
        if (!hWnd) {
            return false;
        }
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);
    SetActiveWindow(hWnd);
    SetForegroundWindow(hWnd);
    return true;
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

static bool RaiseWeChatWindowToForeground_Mac(const wb_process::PID& pid)
{
    throw std::exception("RaiseWeChatWindowToForeground_Mac stub");
    return false;
}

#endif

std::string wxbox::crack::wx::GetWxInstallationPath()
{
#if WXBOX_IN_WINDOWS_OS
    return GetWxInstallationPath_Windows();
#elif WXBOX_IN_MAC_OS
    return GetWxInstallationPath_Mac();
#endif
}

std::string wxbox::crack::wx::GetWxModuleFolderPath(const std::string& installPath)
{
#if WXBOX_IN_WINDOWS_OS
    return GetWxModuleFolderPath_Windows(installPath);
#elif WXBOX_IN_MAC_OS
    return GetWxModuleFolderPath_Mac(installPath);
#endif
}

std::string wxbox::crack::wx::GetWxVersion(const std::string& moduleFolderPath)
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

bool wxbox::crack::wx::IsWxInstallationPathValid(const std::string& installPath)
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

bool wxbox::crack::wx::IsWxInstallationPathValid(const std::string& installPath, const std::string& moduleFolderPath)
{
    if (installPath.empty() || !wb_file::IsPathExists(installPath) || moduleFolderPath.empty() || !wb_file::IsPathExists(moduleFolderPath)) {
        return false;
    }

    if (!wb_file::IsPathExists(wb_file::JoinPath(installPath, WX_WE_CHAT_EXE))) {
        return false;
    }

    return wb_file::IsPathExists(wb_file::JoinPath(moduleFolderPath, WX_WE_CHAT_CORE_MODULE));
}

bool wxbox::crack::wx::ResolveWxEnvInfo(WeChatEnvironmentInfo& wxEnvInfo)
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

bool wxbox::crack::wx::ResolveWxEnvInfo(const wb_process::PID& pid, WeChatEnvironmentInfo& wxEnvInfo)
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

bool wxbox::crack::wx::ResolveWxEnvInfo(const wxbox::util::process::PID& pid, WeChatProcessEnvironmentInfo& wxProcessEnvInfo)
{
    std::string installPath = wb_file::GetProcessRootPath(pid);
    if (installPath.empty()) {
        return false;
    }

    wb_process::ModuleInfo coreModuleInfo;
    if (!wb_process::GetModuleInfo(pid, WX_WE_CHAT_CORE_MODULE, coreModuleInfo)) {
        return false;
    }

    wxProcessEnvInfo.pid                 = pid;
    wxProcessEnvInfo.pCoreModuleBaseAddr = coreModuleInfo.pModuleBaseAddr;
    wxProcessEnvInfo.uCoreModuleSize     = coreModuleInfo.uModuleSize;

    return ResolveWxEnvInfo(installPath, wb_file::ToDirectoryPath(coreModuleInfo.modulePath), wxProcessEnvInfo.wxEnvInfo);
}

bool wxbox::crack::wx::ResolveWxEnvInfo(const std::string& installPath, WeChatEnvironmentInfo& wxEnvInfo)
{
    return ResolveWxEnvInfo(installPath, GetWxModuleFolderPath(installPath), wxEnvInfo);
}

bool wxbox::crack::wx::ResolveWxEnvInfo(const std::string& installPath, const std::string& moduleFolderPath, WeChatEnvironmentInfo& wxEnvInfo)
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

std::vector<wb_process::ProcessInfo> wxbox::crack::wx::GetWeChatProcessList()
{
    auto processLists = wxbox::util::process::GetProcessList();

    processLists.erase(std::remove_if(processLists.begin(), processLists.end(), [&](const wb_process::ProcessInfo& pi) {
                           return ::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
                       }),
                       processLists.end());

    return processLists;
}

std::vector<wxbox::util::process::PID> wxbox::crack::wx::GetWeChatProcessIdList()
{
#if WXBOX_IN_WINDOWS_OS
    std::vector<wb_process::PID> vt;
    HANDLE                       hSnapshot = NULL;
    PROCESSENTRY32               pe32      = {0};

    hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return vt;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!::Process32First(hSnapshot, &pe32)) {
        ::CloseHandle(hSnapshot);
        return vt;
    }

    do {
        if (::_stricmp(pe32.szExeFile, WX_WE_CHAT_EXE)) {
            continue;
        }

        vt.push_back(pe32.th32ProcessID);
    } while (::Process32Next(hSnapshot, &pe32));

    ::CloseHandle(hSnapshot);

    return vt;
#else
    throw std::exception("GetWeChatProcessIdList stub");
#endif
}

bool wxbox::crack::wx::CheckWeChatProcessValid(wxbox::util::process::PID pid)
{
    wb_process::ProcessInfo pi;
    if (!wb_process::GetProcessInfoByPID(pid, pi)) {
        return false;
    }

    return !::_stricmp(pi.filename.c_str(), WX_WE_CHAT_EXE);
}

bool wxbox::crack::wx::RaiseWeChatWindowToForeground(const wb_process::PID& pid)
{
#if WXBOX_IN_WINDOWS_OS
    return RaiseWeChatWindowToForeground_Windows(pid);
#elif WXBOX_IN_MAC_OS
    return RaiseWeChatWindowToForeground_Mac(pid);
#endif
}