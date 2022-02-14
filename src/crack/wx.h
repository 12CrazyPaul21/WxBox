#ifndef __WXBOX_CRACK_WX_H
#define __WXBOX_CRACK_WX_H

namespace wxbox {
    namespace crack {
        namespace wx {

            //
            // Macro
            //

#if WXBOX_IN_WINDOWS_OS
#define WX_WE_CHAT_EXE "WeChat.exe"
#define WX_WE_CHAT_CORE_MODULE "WeChatWin.dll"
#elif WXBOX_IN_MAC_OS
#define WX_WE_CHAT_EXE ""
#define WX_WE_CHAT_CORE_MODULE ""
#endif

            //
            // Typedef
            //

            typedef struct _WeChatEnvironmentInfo
            {
                std::string installPath;
                std::string executeFileName;
                std::string executeAbsPath;
                std::string moduleFolderAbsPath;
                std::string coreModuleName;
                std::string coreModuleAbsPath;
                std::string version;

                _WeChatEnvironmentInfo() = default;

                SETUP_COPY_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath         = other.installPath;
                    executeFileName     = other.executeFileName;
                    executeAbsPath      = other.executeAbsPath;
                    moduleFolderAbsPath = other.moduleFolderAbsPath;
                    coreModuleName      = other.coreModuleName;
                    coreModuleAbsPath   = other.coreModuleAbsPath;
                    version             = other.version;
                }

                SETUP_MOVE_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath         = std::move(other.installPath);
                    executeFileName     = std::move(other.executeFileName);
                    executeAbsPath      = std::move(other.executeAbsPath);
                    moduleFolderAbsPath = std::move(other.moduleFolderAbsPath);
                    coreModuleName      = std::move(other.coreModuleName);
                    coreModuleAbsPath   = std::move(other.coreModuleAbsPath);
                    version             = std::move(other.version);
                }

            } WeChatEnvironmentInfo, *PWeChatEnvironmentInfo;

            //
            // Function
            //

            std::string GetWxInstallationPath();
            std::string GetWxModuleFolderPath(const std::string& installPath);

            std::string GetWxVersion(const std::string& moduleFolderPath);

            bool IsWxInstallationPathValid(const std::string& installPath);
            bool IsWxInstallationPathValid(const std::string& installPath, const std::string& moduleFolderPath);

            bool ResolveWxEnvInfo(WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const wxbox::util::process::PID& pid, WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const std::string& installPath, WeChatEnvironmentInfo& wxEnvInfo);
            bool ResolveWxEnvInfo(const std::string& installPath, const std::string& moduleFolderPath, WeChatEnvironmentInfo& wxEnvInfo);

            std::vector<wxbox::util::process::ProcessInfo> GetWeChatProcessList();
            bool                                           CheckWeChatProcessValid(wxbox::util::process::PID pid);
        }
    }
}

#endif  // #ifndef __WXBOX_CRACK_WX_H