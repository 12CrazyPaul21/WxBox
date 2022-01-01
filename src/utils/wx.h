#ifndef __WXBOX_UTILS_WX_H
#define __WXBOX_UTILS_WX_H

namespace wxbox {
    namespace util {
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
                std::string version;

                _WeChatEnvironmentInfo()
                {
                }

                SETUP_COPY_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath     = other.installPath;
                    executeFileName = other.executeFileName;
                    executeAbsPath  = other.executeAbsPath;
                    version         = other.version;
                }

                SETUP_MOVE_METHOD(_WeChatEnvironmentInfo, other)
                {
                    installPath     = std::move(other.installPath);
                    executeFileName = std::move(other.executeFileName);
                    executeAbsPath  = std::move(other.executeAbsPath);
                    version         = std::move(other.version);
                }

            } WeChatEnvironmentInfo, *PWeChatEnvironmentInfo;

            //
            // Function
            //

            std::string GetWxInstallationPath();
            bool        IsWxInstallationPathValid(const std::string& installPath);
            std::string GetWxVersion(const std::string& installPath);
            bool        ResolveWxEnvInfo(const std::string& installPath, PWeChatEnvironmentInfo pWxEnvInfo);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_WX_H