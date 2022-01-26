#ifndef __WXBOX_UTILS_PLATFORM_H
#define __WXBOX_UTILS_PLATFORM_H

namespace wxbox {
    namespace util {
        namespace platform {

            //
            // Function
            //

            bool        Is64System();
            std::string GetCPUProductBrandDescription();
            std::string GetSystemVersionDescription();

            //
            // only for windows
            //

#if WXBOX_IN_WINDOWS_OS
            bool        EnableDebugPrivilege(bool bEnablePrivilege = true);
            std::string GetStringValueInRegister(HKEY hKey, const char* subKey, const char* valueName);
            const char* ExceptionDescription(const DWORD code);
            ucpulong_t  GetPEModuleImageSize(const std::string& path);
#endif
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_PLATFORM_H