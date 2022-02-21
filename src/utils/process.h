#ifndef __WXBOX_UTILS_PROCESS_H
#define __WXBOX_UTILS_PROCESS_H

namespace wxbox {
    namespace util {

        namespace memory {
            template<class T>
            struct internal_allocator;
        }

        namespace process {

            //
            // Typedef
            //

#if WXBOX_IN_WINDOWS_OS
            typedef HWND    WIN_HANDLE;
            typedef POINT   SCREEN_POINT;
            typedef DWORD   PID;
            typedef DWORD   TID;
            typedef HANDLE  PROCESS_HANDLE;
            typedef HANDLE  THREAD_HANDLE;
            typedef HMODULE MODULE_HANDLE;
            typedef HANDLE  MUTEX_HANDLE;
            typedef HANDLE  SHARED_HANDLE;
#elif WXBOX_IN_MAC_OS
            /*
			typedef ucpulong_t WIN_HANDLE;
			typedef struct
			{
                int32_t x;
                int32_t y;
			} SCREEN_POINT;
			typedef pid_t PID;
			typedef pid_t TID;
			typedef ucpulong_t PROCESS_HANDLE;
			typedef ucpulong_t THREAD_HANDLE;
			typedef ucpulong_t MODULE_HANDLE;
			typedef ucpulong_t MUTEX_HANDLE;
			typedef ucpulong_t SHARED_HANDLE;
			*/
#endif

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

            typedef struct _AutoProcessHandle
            {
                PROCESS_HANDLE hProcess;

                _AutoProcessHandle(const _AutoProcessHandle&) = delete;
                _AutoProcessHandle& operator=(const _AutoProcessHandle& OTHER_VAL_NAME) = delete;

                _AutoProcessHandle()
                  : hProcess(0)
                {}

                ~_AutoProcessHandle()
                {
                    close();
                }

                SETUP_MOVE_METHOD(_AutoProcessHandle, other)
                {
                    hProcess       = other.hProcess;
                    other.hProcess = 0;
                }

                bool valid() const
                {
                    return hProcess != 0;
                }

                void close();

            } AutoProcessHandle;

            typedef struct _ModuleInfo
            {
                MODULE_HANDLE hModule;
                std::string   moduleName;
                std::string   modulePath;
                void*         pModuleBaseAddr;
                ucpulong_t    uModuleSize;
            } ModuleInfo, *PModuleInfo;

            class AppSingleton final
            {
                AppSingleton() = delete;

              public:
                AppSingleton(const std::string& name, bool lock = false);
                ~AppSingleton();

                bool TryLock();
                void Release();

                void RecordWindowId(ucpulong_t id);
                void WakeApplicationWindow();

              private:
                std::string   name;
                std::string   mutexName;
                std::string   winIdSharedName;
                ucpulong_t*   winId;
                MUTEX_HANDLE  mutex;
                SHARED_HANDLE shared;
            };

            typedef struct _CallFrameHitTestItem
            {
                void*      addr;
                ucpulong_t length;
            } CallFrameHitTestItem, *PCallFrameHitTestItem;

            using CallFrameHitTestItemVector = std::vector<CallFrameHitTestItem, wxbox::util::memory::internal_allocator<CallFrameHitTestItem>>;

            //
            // Function
            //

            std::time_t GetCurrentTimestamp(bool ms = true);

            std::vector<ProcessInfo> GetProcessList();

            PID            GetCurrentProcessId();
            PROCESS_HANDLE GetCurrentProcessHandle();

            TID           GetCurrentThreadId();
            THREAD_HANDLE GetCurrentThreadHandle();

            bool              Is64Process(PROCESS_HANDLE hProcess);
            PROCESS_HANDLE    OpenProcessHandle(PID pid);
            AutoProcessHandle OpenProcessAutoHandle(PID pid);
            void              CloseProcessHandle(PROCESS_HANDLE handle);

            WIN_HANDLE              GetWindowHandleFromScreenPoint(const SCREEN_POINT& pt);
            bool                    GetProcessInfoFromWindowHandle(const WIN_HANDLE& hWnd, ProcessInfo& pi);
            bool                    GetModuleInfo(PID pid, const std::string& moduleName, ModuleInfo& moduleInfo);
            std::vector<ModuleInfo> CollectModuleInfos(PID pid);
            bool                    GetProcessInfoByPID(PID pid, ProcessInfo& pi);

            PID  StartProcess(const std::string& binFilePath, bool isAttach);
            bool SuspendAllOtherThread(PID pid, TID tid);
            void ResumeAllThread(PID pid, TID tid = 0);

            void        SetThreadName(THREAD_HANDLE hThread, const std::string& threadName);
            std::string GetThreadName(THREAD_HANDLE hThread);

            std::vector<TID, wxbox::util::memory::internal_allocator<TID>>                            GetAllThreadId(PID pid, TID excludeThreadId = 0);
            std::vector<ucpulong_t, wxbox::util::memory::internal_allocator<ucpulong_t>>              WalkThreadStack(TID tid);
            std::set<ucpulong_t, std::less<int>, wxbox::util::memory::internal_allocator<ucpulong_t>> GetAllOtherThreadCallFrameEips();
            std::vector<ucpulong_t, wxbox::util::memory::internal_allocator<ucpulong_t>>              HitTestAllOtherThreadCallFrame(const CallFrameHitTestItemVector& targets);
            bool                                                                                      HitTestAllOtherThreadCallFrame(void* addr, ucpulong_t length);

            //
            // Async
            //

            // if want to wait for the result, use std::async
            template<typename _FunctionType, typename... _ArgTypes>
            inline void async_task(_FunctionType&& func, _ArgTypes&&... args)
            {
                std::thread(std::forward<_FunctionType>(func), std::forward<_ArgTypes>(args)...).detach();
            }

            //
            // Template Functions
            //

            template<typename TimeType = std::chrono::seconds>
            std::string TimeStampToDate(std::time_t timestamp, bool standard = true)
            {
                auto              timePoint = std::chrono::time_point<std::chrono::system_clock, TimeType>(TimeType(timestamp));
                auto              tt        = std::chrono::system_clock::to_time_t(timePoint);
                std::stringstream ss;

                char      buffer[100] = {0};
                struct tm timeInfo    = {0};

#if WXBOX_IN_WINDOWS_OS
                localtime_s(&timeInfo, &tt);
#else
                localtime_r(&timeInfo, &tt);
#endif

                if (standard) {
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", &timeInfo);
                }
                else {
                    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &timeInfo);
                }
                return buffer;
            }
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_PROCESS_H