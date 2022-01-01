#ifndef __WXBOX_UTILS_WX_H
#define __WXBOX_UTILS_WX_H

namespace wxbox {
    namespace util {
        namespace wx {

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
            bool        OpenWxWithMultiBoxing(const WeChatEnvironmentInfo& wxEnvInfo);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_WX_H