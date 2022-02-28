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

            void LockScreen();
            bool Shell(const std::string& command, const std::vector<std::string>& args = std::vector<std::string>());

            //
            // only for windows
            //

#if WXBOX_IN_WINDOWS_OS
            bool        EnableDebugPrivilege(bool bEnablePrivilege = true);
            std::string GetStringValueInRegister(HKEY hKey, const char* subKey, const char* valueName);
            const char* ExceptionDescription(const DWORD code);
            ucpulong_t  GetPEModuleImageSize(const std::string& path);
            bool        RemoveAllMatchKernelObject(const std::string& programName, const std::wstring& kernelObjectNamePattern);
#endif
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_PLATFORM_H