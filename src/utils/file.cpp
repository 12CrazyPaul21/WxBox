#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

static inline std::string GetProcessRootPath_Windows()
{
    char fullPath[MAX_PATH]    = {0};
    char absFullPath[MAX_PATH] = {0};

    if (!::GetModuleFileNameA(NULL, fullPath, MAX_PATH)) {
        return "";
    }

    if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
        return "";
    }

    return std::move(wxbox::util::file::ToDirectoryPath(absFullPath));
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetProcessRootPath_Mac()
{
    return "";
}

#endif

bool wxbox::util::file::IsPathExists(const std::string& path)
{
    if (path.length() == 0) {
        return false;
    }

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    return ::PathFileExistsA(path.c_str());
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    struct stat sbuf = {0};
    if (stat(path.c_str(), &sbuf)) {
        return ENOENT != errno;
    }
    return true;
#endif
}

bool wxbox::util::file::IsDirectory(const std::string& path)
{
    if (path.length() == 0) {
        return false;
    }

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    return ::PathIsDirectoryA(path.c_str());
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    struct stat sbuf = {0};
    if (stat(path.c_str(), &sbuf)) {
        return false;
    }
    return S_ISDIR(sbuf.st_mode);
#endif
}

std::string wxbox::util::file::ToDirectoryPath(const std::string& path)
{
    if (path.length() == 0) {
        return "";
    }

    if (IsDirectory(path)) {
        return path;
    }

    return std::move(std::experimental::filesystem::path(path).parent_path().string());
}

std::string wxbox::util::file::JoinPath(const std::string& dirPath, const std::string& fileName)
{
#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    char dest[MAX_PATH] = {0};
    PathCombineA(dest, dirPath.c_str(), fileName.c_str());
    return dest;
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    namespace fs = std::experimental::filesystem;
    return std::move((fs::path(dirPath) / fs::path(fileName)).string());
#endif
}

std::string wxbox::util::file::GetProcessRootPath()
{
    std::string processRootPath = "";

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    processRootPath = GetProcessRootPath_Windows();
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    processRootPath = GetProcessRootPath_Mac();
#endif

    return processRootPath;
}