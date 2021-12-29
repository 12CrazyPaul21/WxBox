#ifndef __WXBOX_UTILS_FILE_H
#define __WXBOX_UTILS_FILE_H

namespace wxbox {
    namespace util {
        namespace file {
            bool        IsPathExists(const std::string& path);
            bool        IsDirectory(const std::string& path);
            std::string ToFileName(const std::string& path);
            std::string ToDirectoryPath(const std::string& path);
            std::string JoinPath(const std::string& dirPath, const std::string& fileName);
            std::string GetProcessRootPath();
        }
    }
}

#endif // #ifndef __WXBOX_UTILS_FILE_H