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

#if WXBOX_IN_WINDOWS_OS
    return ::PathFileExistsA(path.c_str());
#elif WXBOX_IN_MAC_OS
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

#if WXBOX_IN_WINDOWS_OS
    return ::PathIsDirectoryA(path.c_str());
#elif WXBOX_IN_MAC_OS
    struct stat sbuf = {0};
    if (stat(path.c_str(), &sbuf)) {
        return false;
    }
    return S_ISDIR(sbuf.st_mode);
#endif
}

std::string wxbox::util::file::ToFileName(const std::string& path)
{
    if (path.length() == 0) {
        return "";
    }

    return std::move(std::experimental::filesystem::path(path).filename().string());
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
#if WXBOX_IN_WINDOWS_OS
    char dest[MAX_PATH] = {0};
    char full[MAX_PATH] = {0};

    if (::PathCombineA(dest, dirPath.c_str(), fileName.c_str())) {
        ::GetFullPathNameA(dest, MAX_PATH, full, nullptr);
    }

    return full;
#elif WXBOX_IN_MAC_OS
    namespace fs = std::experimental::filesystem;
    return std::move((fs::path(dirPath) / fs::path(fileName)).string());
#endif
}

std::string wxbox::util::file::GetProcessRootPath()
{
    std::string processRootPath = "";

#if WXBOX_IN_WINDOWS_OS
    processRootPath = GetProcessRootPath_Windows();
#elif WXBOX_IN_MAC_OS
    processRootPath = GetProcessRootPath_Mac();
#endif

    return processRootPath;
}

YAML::Node wxbox::util::file::UnwindYamlFile(const std::string& path)
{
    YAML::Node root;

    // load and parse yaml file
    try {
        root = YAML::LoadFile(path);
    }
    catch (const YAML::BadFile& /*e*/) {
        // file cannot be loaded
    }
    catch (const YAML::ParserException& /*e*/) {
        // parse failed
    }

    return std::move(root);
}

bool wxbox::util::file::UnwindVersionNumber(const std::string& version, wxbox::util::file::VersionNumber& versionNumber)
{
    versionNumber.major    = 0;
    versionNumber.minor    = 0;
    versionNumber.revision = 0;
    versionNumber.build    = 0;

    //
    // note: c++11's regex doesn't support lookbehind
    // ^([0-9]+(\\.)?){1,4}(?<=[^\\.])$
    //
    // ^(([0-9]+)\\.){0,3}([0-9]+){1}$
    // ^([0-9]+)?\\.?([0-9]+)?\\.?([0-9]+)?\\.?([0-9]+)$
    // ^(?:([0-9]+)\\.)?(?:([0-9]+)\\.)?(?:([0-9]+)\\.)?([0-9]+)$
    //

    std::regex matchPattern("^(?:([0-9]+)\\.)?(?:([0-9]+)\\.)?(?:([0-9]+)\\.)?([0-9]+)$");

    std::smatch result;
    if (!std::regex_match(version.begin(), version.end(), result, matchPattern)) {
        return false;
    }
    if (result.size() != 5) {
        return false;
    }

    //
    // split version number
    //

    uint32_t parts[4]  = {0, 0, 0, 0};
    int      partIndex = 0;

    for (size_t i = 1; i < 5; i++) {
        if (!result[i].matched) {
            continue;
        }

#if WXBOX_PLATFORM != WXBOX_WINDOWS_OS
#define _atoi64(val) strtoll(val, nullptr, 100)
#endif

        uint32_t number    = (uint32_t)_atoi64(result[i].str().c_str());
        parts[partIndex++] = number;
    }

    //
    // fill version info
    //

    versionNumber.major    = parts[0];
    versionNumber.minor    = parts[1];
    versionNumber.revision = parts[2];
    versionNumber.build    = parts[3];
    versionNumber.str      = version;

    return true;
}