#ifndef __WXBOX_UTILS_FILE_H
#define __WXBOX_UTILS_FILE_H

namespace wxbox {
    namespace util {
        namespace file {

            //
            // Typedef
            //

            typedef struct _VersionNumber
            {
                // Major Version Number
                uint32_t major;

                // Minor Version Number
                uint32_t minor;

                // Revision Number
                uint32_t revision;

                // Build Number
                uint32_t build;

                // Raw String
                std::string str;

                _VersionNumber();

                int  compare(const _VersionNumber& right);
                bool operator==(const _VersionNumber& right);
                bool operator!=(const _VersionNumber& right);
                bool operator>(const _VersionNumber& right);
                bool operator>=(const _VersionNumber& right);
                bool operator<(const _VersionNumber& right);
                bool operator<=(const _VersionNumber& right);

            } VersionNumber, *PVersionNumber;

            typedef enum class _FileChangeType
            {
                Invalid = 0,
                Added,
                Modified,
                Removed,
                Renamed
            } FileChangeType;

            typedef struct _FileChangeMonitorReport
            {
                FileChangeType type;
                std::string    dirpath;
                std::string    fullpath;
                std::string    filename;
                std::string    oldname;

                _FileChangeMonitorReport()
                  : type(FileChangeType::Invalid)
                  , dirpath("")
                  , fullpath("")
                  , filename("")
                  , oldname("")
                {
                }

                SETUP_COPY_METHOD(_FileChangeMonitorReport, other)
                {
                    type     = other.type;
                    dirpath  = other.dirpath;
                    fullpath = other.fullpath;
                    filename = other.filename;
                    oldname  = other.oldname;
                }

                SETUP_MOVE_METHOD(_FileChangeMonitorReport, other)
                {
                    type     = other.type;
                    dirpath  = std::move(other.dirpath);
                    fullpath = std::move(other.fullpath);
                    filename = std::move(other.filename);
                    oldname  = std::move(other.oldname);
                }

            } FileChangeMonitorReport, *PFileChangeMonitorReport;

            typedef struct _FileChangeMonitorContext
            {
                std::string        dirpath;
                std::promise<void> closeSignal;
                std::future<void>  finishFuture;

                _FileChangeMonitorContext(const _FileChangeMonitorContext&) = delete;
                _FileChangeMonitorContext& operator=(const _FileChangeMonitorContext&) = delete;

                _FileChangeMonitorContext()
                  : dirpath("")
                {
                }

                SETUP_MOVE_METHOD(_FileChangeMonitorContext, other)
                {
                    dirpath      = std::move(other.dirpath);
                    closeSignal  = std::move(other.closeSignal);
                    finishFuture = std::move(other.finishFuture);
                }

            } FileChangeMonitorContext, *PFileChangeMonitorContext;

            using FileChangeMonitorCallback = std::function<void(FileChangeMonitorReport report)>;

            //
            // Function
            //

            bool                                IsPathExists(const std::string& path);
            bool                                IsDirectory(const std::string& path);
            std::string                         ToFileName(const std::string& path);
            std::string                         ToDirectoryPath(const std::string& path);
            std::string                         JoinPath(const std::string& dirPath, const std::string& fileName);
            std::pair<std::string, std::string> ExtractFileNameAndExt(const std::string& path);
            std::string                         GetProcessRootPath();

            std::vector<std::string> ListFilesInDirectoryWithExt(const std::string& dirPath, const std::string& ext);

            bool OpenFolderFilesChangeMonitor(const std::string& dirPath, const FileChangeMonitorCallback& callback);
            void CloseFolderFilesChangeMonitor(const std::string& dirPath);
            void CloseFolderFilesChangeMonitor();

            std::string GetFileVersion(const std::string& path);
            std::time_t GetFileModifyTimestamp(const std::string& path);

            YAML::Node UnwindYamlFile(const std::string& path);
            bool       UnwindVersionNumber(const std::string& version, VersionNumber& versionNumber);

            int  ExposeFileStreamFD(std::filebuf* fb);
            void OpenFolderInExplorer(const std::string& path);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_FILE_H