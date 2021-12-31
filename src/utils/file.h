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

                _VersionNumber()
                  : major(0)
                  , minor(0)
                  , revision(0)
                  , build(0)
                {
                }

            } VersionNumber, *PVersionNumber;

            //
            // Function
            //

            bool        IsPathExists(const std::string& path);
            bool        IsDirectory(const std::string& path);
            std::string ToFileName(const std::string& path);
            std::string ToDirectoryPath(const std::string& path);
            std::string JoinPath(const std::string& dirPath, const std::string& fileName);
            std::string GetProcessRootPath();
            YAML::Node  UnwindYamlFile(const std::string& path);
            bool        UnwindVersionNumber(const std::string& version, VersionNumber& versionNumber);
        }
    }
}

#endif // #ifndef __WXBOX_UTILS_FILE_H