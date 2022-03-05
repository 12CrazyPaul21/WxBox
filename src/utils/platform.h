#ifndef __WXBOX_UTILS_PLATFORM_H
#define __WXBOX_UTILS_PLATFORM_H

namespace wxbox {
    namespace util {
        namespace platform {

            //
            // Typedef
            //

            typedef struct _MonitorPhysicalRegion
            {
                long          x;
                long          y;
                unsigned long width;
                unsigned long height;

                _MonitorPhysicalRegion()
                  : x(0)
                  , y(0)
                  , width(0)
                  , height(0)
                {
                }

                _MonitorPhysicalRegion(long x, long y, unsigned long width, unsigned long height)
                  : x(x)
                  , y(y)
                  , width(width)
                  , height(height)
                {
                }
            } MonitorPhysicalRegion, FullMonitorPhysicalRegion, *PMonitorPhysicalRegion;

            typedef struct _MultiMonitorRegionInfo
            {
                long                               min_x;
                long                               min_y;
                long                               max_x;
                long                               max_y;
                unsigned long                      min_width;
                unsigned long                      min_height;
                unsigned long                      max_width;
                unsigned long                      max_height;
                std::vector<MonitorPhysicalRegion> monitorRegions;

                _MultiMonitorRegionInfo()
                  : min_x(0)
                  , min_y(0)
                  , max_x(0)
                  , max_y(0)
                  , min_width(0)
                  , min_height(0)
                  , max_width(0)
                  , max_height(0)
                {
                }

                bool valid()
                {
                    return !monitorRegions.empty();
                }

                FullMonitorPhysicalRegion region()
                {
                    MonitorPhysicalRegion fullRegion;

                    if (!valid()) {
                        return fullRegion;
                    }

                    fullRegion.x      = min_x;
                    fullRegion.y      = min_y;
                    fullRegion.height = max_height;
                    for (auto r : monitorRegions) {
                        fullRegion.width += r.width;
                    }
                    return fullRegion;
                }

            } MultiMonitorRegionInfo, *PMultiMonitorRegionInfo;

            //
            // Function
            //

            bool        Is64System();
            std::string GetCPUProductBrandDescription();
            std::string GetSystemVersionDescription();

            void LockScreen();
            bool CaptureMainMonitorSnap(const std::string& savePngImageFilePath);
            bool CaptureMonitorSnap(const std::string& savePngImageFilePath);
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