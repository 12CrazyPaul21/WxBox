#ifndef __WXBOX_UTILS_COREDUMP_H
#define __WXBOX_UTILS_COREDUMP_H

namespace wxbox {
    namespace util {
        namespace coredump {

            //
            // Typedef
            //

            using ExceptionExitCallback = std::function<void()>;

            //
            // Function
            //

            bool RegisterUnhandledExceptionAutoDumper(const std::string& dumpPrefix, const std::string& dumpSinkPath, const std::string& dumperPath, const std::string& i18nPath, const std::string& themePath, bool disabledWhenDebug = true);
            void ChangeDumperLanguage(const std::string& language);
            void ChangeTheme(const std::string& themeName);
            void RegisterExceptionExitCallback(ExceptionExitCallback callback);

#if WXBOX_IN_WINDOWS_OS

            PRAGMA(pack(push, 1))

            typedef struct CrashDumperRequest
            {
                bool                is64Process;
                uint64_t            pid;
                uint64_t            tid;
                EXCEPTION_POINTERS* exception;

                char imageName[MAX_PATH + 1];
                char binRoot[MAX_PATH + 1];

                char dumpPrefix[MAX_PATH + 1];
                char dumpSinkPath[MAX_PATH + 1];

                char i18nPath[MAX_PATH + 1];
                char language[MAX_PATH + 1];

                char themePath[MAX_PATH + 1];
                char themeName[MAX_PATH + 1];

                char crashTimestamp[MAX_PATH + 1];
                char crashDate[MAX_PATH + 1];
                char finishEventName[MAX_PATH + 1];
            } CrashDumperRequest, *PCrashDumperRequest;

            PRAGMA(pack(pop))

            typedef struct GenerateMiniDumpParameter
            {
                HANDLE              hProcess;
                DWORD               pid;
                DWORD               tid;
                BOOL                bClientPointers;
                EXCEPTION_POINTERS* exception;

                std::string dumpSinkPath;
                std::string dumpPrefix;

                std::string timestamp;
            } GenerateMiniDumpParameter, *PGenerateMiniDumpParameter;

            bool GenerateMiniDump(const GenerateMiniDumpParameter& parameter);
#endif
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_COREDUMP_H