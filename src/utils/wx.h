#ifndef __WXBOX_UTILS_WX_H
#define __WXBOX_UTILS_WX_H

#include <string>

namespace wxbox {
    namespace util{
		namespace wx {
            std::string GetWxInstallationPath();
            bool        IsWxInstallationPathValid(const std::string& installPath);
            std::string GetWxVersion(const std::string& installPath);
		}
    }
}

#endif // #ifndef __WXBOX_UTILS_WX_H