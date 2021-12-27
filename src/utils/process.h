#ifndef __WXBOX_UTILS_PROCESS_H
#define __WXBOX_UTILS_PROCESS_H

namespace wxbox {
    namespace util {
        namespace process {

            //
            // typedef
            //

            typedef struct _ProcessInfo
            {
                std::string abspath;
                std::string filename;
                std::string dirpath;
                uint32_t    pid;

                _ProcessInfo()
                  : pid(0)
                {
                }

                _ProcessInfo(const _ProcessInfo& other)
                {
                    abspath  = other.abspath;
                    filename = other.filename;
                    dirpath  = other.dirpath;
                    pid      = other.pid;
                }

                _ProcessInfo& operator=(const _ProcessInfo& other)
                {
                    abspath  = other.abspath;
                    filename = other.filename;
                    dirpath  = other.dirpath;
                    pid      = other.pid;
                }

                _ProcessInfo(_ProcessInfo&& other)
                {
                    abspath  = std::move(other.abspath);
                    filename = std::move(other.filename);
                    dirpath  = std::move(other.dirpath);
                    pid      = other.pid;
                }

                _ProcessInfo& operator=(const _ProcessInfo&& other)
                {
                    abspath  = std::move(other.abspath);
                    filename = std::move(other.filename);
                    dirpath  = std::move(other.dirpath);
                    pid      = other.pid;
                }
            } ProcessInfo, PProcessInfo;

#if WXBOX_IN_WINDOWS_OS
            typedef HWND  WIN_HANDLE;
            typedef POINT SCREEN_POINT;
#elif WXBOX_IN_MAC_OS
            /*typedef uint64_t WIN_HANDLE;
			typedef struct
			{
                int32_t x;
                int32_t y;
			} SCREEN_POINT;*/
#endif

            //
            // Function
            //

            std::vector<ProcessInfo> GetProcessList();

			WIN_HANDLE GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt);
            bool       GetProcessInfoFromWindowHandle(const WIN_HANDLE& hWnd, ProcessInfo& pi);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_PROCESS_H