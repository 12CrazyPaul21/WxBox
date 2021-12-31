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

                _VersionNumber()
                  : major(0)
                  , minor(0)
                  , revision(0)
                  , build(0)
                  , str("")
                {
                }

                int compare(const _VersionNumber& right)
                {
                    uint32_t leftVersion[4]  = {major, minor, revision, build};
                    uint32_t rightVersion[4] = {right.major, right.minor, right.revision, right.build};

                    for (int i = 0; i < 4; i++) {
                        if (leftVersion[i] > rightVersion[i]) {
                            return 1;
                        }
                        else if (leftVersion[i] < rightVersion[i]) {
                            return -1;
                        }
                    }

                    return 0;
                }

                bool operator==(const _VersionNumber& right)
                {
                    return major == right.major && minor == right.minor && revision == right.revision && build == right.build;
                }

                bool operator!=(const _VersionNumber& right)
                {
                    return !operator==(right);
                }

                bool operator>(const _VersionNumber& right)
                {
                    return compare(right) == 1;
                }

                bool operator>=(const _VersionNumber& right)
                {
                    return compare(right) >= 0;
                }

                bool operator<(const _VersionNumber& right)
                {
                    return compare(right) == -1;
                }

                bool operator<=(const _VersionNumber& right)
                {
                    return compare(right) <= 0;
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

#endif  // #ifndef __WXBOX_UTILS_FILE_H