#ifndef __WXBOX_UTILS_PROCESS_H
#define __WXBOX_UTILS_PROCESS_H

namespace wxbox {
    namespace util {
        namespace process {

            //
            // Typedef
            //

            typedef struct _ProcessInfo
            {
                std::string abspath;
                std::string filename;
                std::string dirpath;
                ucpulong_t  pid;

                _ProcessInfo()
                  : pid(0)
                {
                }

                SETUP_COPY_METHOD(_ProcessInfo, other)
                {
                    abspath  = other.abspath;
                    filename = other.filename;
                    dirpath  = other.dirpath;
                    pid      = other.pid;
                }

                SETUP_MOVE_METHOD(_ProcessInfo, other)
                {
                    abspath  = std::move(other.abspath);
                    filename = std::move(other.filename);
                    dirpath  = std::move(other.dirpath);
                    pid      = other.pid;
                }

            } ProcessInfo, *PProcessInfo;

            typedef struct _ModuleInfo
            {
                std::string moduleName;
                std::string modulePath;
                void*       pModuleBaseAddr;
                ucpulong_t  uModuleSize;
            } ModuleInfo, *PModuleInfo;

#if WXBOX_IN_WINDOWS_OS
            typedef HWND   WIN_HANDLE;
            typedef POINT  SCREEN_POINT;
            typedef DWORD  PID;
            typedef HANDLE PROCESS_HANDLE;
#elif WXBOX_IN_MAC_OS
            /*
			typedef ucpulong_t WIN_HANDLE;
			typedef struct
			{
                int32_t x;
                int32_t y;
			} SCREEN_POINT;
			typedef ucpulong_t PID;
			typedef ucpulong_t PROCESS_HANDLE;
			*/
#endif

            //
            // Function
            //

            std::vector<ProcessInfo> GetProcessList();
            PID                      GetCurrentProcessId();

            WIN_HANDLE GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt);
            bool       GetProcessInfoFromWindowHandle(const WIN_HANDLE& hWnd, ProcessInfo& pi);
            bool       GetModuleInfo(PID pid, const std::string& moduleName, ModuleInfo& moduleInfo);
            bool       GetProcessInfoByPID(PID pid, ProcessInfo& pi);

            PID StartProcessAndAttach(const std::string& binFilePath);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_PROCESS_H