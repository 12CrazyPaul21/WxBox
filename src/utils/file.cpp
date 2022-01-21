#include <utils/common.h>

//
// Global variables
//

static std::unordered_map<std::string, wb_file::FileChangeMonitorContext> g_fileMonitorRecords;
static std::mutex                                                         g_fileMonitorMutex;

static constexpr DWORD FOLDER_FILES_CHANGE_MONITOR_INTERVAL_MS                = 500;
static constexpr DWORD FOLDER_FILES_CHANGE_MONITOR_WAIT_FOR_FINISH_TIMEOUT_MS = 10000;

//
// Classes or Structures
//

/**
 * wxbox::util::file::VersionNumber
 */
wb_file::_VersionNumber::_VersionNumber()
  : major(0)
  , minor(0)
  , revision(0)
  , build(0)
  , str("")
{
}

int wb_file::_VersionNumber::compare(const _VersionNumber& right)
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

bool wb_file::_VersionNumber::operator==(const _VersionNumber& right)
{
    return major == right.major && minor == right.minor && revision == right.revision && build == right.build;
}

bool wb_file::_VersionNumber::operator!=(const _VersionNumber& right)
{
    return !operator==(right);
}

bool wb_file::_VersionNumber::operator>(const _VersionNumber& right)
{
    return compare(right) == 1;
}

bool wb_file::_VersionNumber::operator>=(const _VersionNumber& right)
{
    return compare(right) >= 0;
}

bool wb_file::_VersionNumber::operator<(const _VersionNumber& right)
{
    return compare(right) == -1;
}

bool wb_file::_VersionNumber::operator<=(const _VersionNumber& right)
{
    return compare(right) <= 0;
}

//
// Functions
//

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

static inline std::vector<std::string> ListFilesInDirectoryWithExt_Windows(const std::string& dirPath, const std::string& ext)
{
    std::vector<std::string> result;

    if (!wb_file::IsDirectory(dirPath)) {
        return std::move(result);
    }

    WIN32_FIND_DATAA findFileData = {0};
    HANDLE           hFind        = ::FindFirstFileA(wb_file::JoinPath(dirPath, R"(*.)" + ext).c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return std::move(result);
    }

    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        result.emplace_back(std::string(findFileData.cFileName));
    } while (::FindNextFileA(hFind, &findFileData));

    return std::move(result);
}

void WINAPI FolderFilesChangeMonitorCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    // do nothing
}

static inline void ReportFileChangeAsync(const wb_file::FileChangeMonitorCallback& callback, const wb_file::FileChangeMonitorReport& report)
{
    if (!callback) {
        return;
    }

    wb_process::async_task(callback, report);
}

static bool OpenFolderFilesChangeMonitor_Windows(const std::string& dirPath, const wb_file::FileChangeMonitorCallback& callback)
{
    static constexpr size_t FOLDER_FILES_CHANGE_MONITOR_BUFFER_SIZE = sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH * 10;

    if (!wb_file::IsDirectory(dirPath) || !callback) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_fileMonitorMutex);

    // cannot monitor the same folder at the same time
    if (std::find_if(g_fileMonitorRecords.begin(), g_fileMonitorRecords.end(), [&dirPath](const decltype(g_fileMonitorRecords)::value_type& item) {
            return !::_stricmp(item.first.c_str(), dirPath.c_str());
        }) != g_fileMonitorRecords.end()) {
        return false;
    }

    // open directory
    HANDLE hDirectory = CreateFileA(dirPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE /* | FILE_SHARE_DELETE*/, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (hDirectory == INVALID_HANDLE_VALUE) {
        return false;
    }

    // run monitor in async task
    wb_file::FileChangeMonitorContext context;
    context.dirpath      = dirPath;
    context.finishFuture = std::move(std::async(
        std::launch::async, [dirPath, callback, hDirectory](std::future<void> closeFuture) {
            uint8_t                  notifyBuf[FOLDER_FILES_CHANGE_MONITOR_BUFFER_SIZE];
            FILE_NOTIFY_INFORMATION* pNotifyInfo        = (FILE_NOTIFY_INFORMATION*)notifyBuf;
            decltype(pNotifyInfo)    pNotifyInfoCursor  = nullptr;
            DWORD                    dwBytesReturned    = 0;
            OVERLAPPED               overlapped         = {0};
            DWORD                    dwLastAction       = 0;
            std::string              lastActionFileName = "";

            // reset notify buffer
            ::memset(notifyBuf, 0, sizeof(notifyBuf));

            /**
		     * ReadDirectoryChangesW is very sensitive to processing time and is not 100% able to receive events
		     * perhaps can use "Change Journal" for higher accuracy
		     */

            while (closeFuture.wait_for(std::chrono::microseconds(10)) == std::future_status::timeout) {
                // read directory changes with completion i/o
                if (!::ReadDirectoryChangesW(hDirectory, &notifyBuf, sizeof(notifyBuf), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, &dwBytesReturned, &overlapped, FolderFilesChangeMonitorCompletionRoutine)) {
                    continue;
                }

                // wait for completion routine finish
                if (::WaitForSingleObject(hDirectory, FOLDER_FILES_CHANGE_MONITOR_INTERVAL_MS) == WAIT_TIMEOUT) {
                    continue;
                }

                // get overlapped result
                if (!::GetOverlappedResult(hDirectory, &overlapped, &dwBytesReturned, TRUE)) {
                    continue;
                }

                pNotifyInfoCursor = pNotifyInfo;
                while (pNotifyInfoCursor) {
                    wchar_t filename[MAX_PATH];
                    ::memset(filename, 0, sizeof(filename));
                    ::memcpy_s(filename, sizeof(filename), pNotifyInfoCursor->FileName, pNotifyInfoCursor->FileNameLength);

                    wb_file::FileChangeMonitorReport report;
                    report.dirpath  = dirPath;
                    report.filename = wb_string::ToString(std::wstring(filename));
                    report.fullpath = wb_file::JoinPath(report.dirpath, report.filename);

                    switch (pNotifyInfoCursor->Action) {
                        case FILE_ACTION_ADDED:
                            report.type = wb_file::FileChangeType::Added;
                            break;
                        case FILE_ACTION_REMOVED:
                            report.type = wb_file::FileChangeType::Removed;
                            break;
                        case FILE_ACTION_MODIFIED:
                            report.type = wb_file::FileChangeType::Modified;
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                            // do nothing
                            break;
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            if (dwLastAction == FILE_ACTION_RENAMED_OLD_NAME) {
                                report.type    = wb_file::FileChangeType::Renamed;
                                report.oldname = lastActionFileName;
                            }
                            break;
                    }

                    dwLastAction       = pNotifyInfoCursor->Action;
                    lastActionFileName = report.filename;

                    // report file change
                    if (report.type != wb_file::FileChangeType::Invalid) {
                        ReportFileChangeAsync(callback, report);
                    }

                    // next record
                    if (!pNotifyInfoCursor->NextEntryOffset) {
                        break;
                    }
                    pNotifyInfoCursor = reinterpret_cast<FILE_NOTIFY_INFORMATION*>((uint8_t*)pNotifyInfoCursor + pNotifyInfoCursor->NextEntryOffset);
                }
            }

            ::CloseHandle(hDirectory);
        },
        std::move(context.closeSignal.get_future())));

    // record
    g_fileMonitorRecords[dirPath] = std::move(context);

    return true;
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetProcessRootPath_Mac()
{
    return "";
}

static inline std::vector<std::string> ListFilesInDirectoryWithExt_Mac(const std::string& dirPath, const std::string& ext)
{
    return std::move(std::vector<std::string>());
}

static bool OpenFolderFilesChangeMonitor_Mac(const std::string& dirPath, const wb_file::FileChangeMonitorCallback& callback)
{
    return false;
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

std::pair<std::string, std::string> wxbox::util::file::ExtractFileNameAndExt(const std::string& path)
{
    std::pair<std::string, std::string> result;

    if (path.empty()) {
        return result;
    }

#if WXBOX_IN_WINDOWS_OS

    char filename[MAX_PATH] = {0};
    char extname[MAX_PATH]  = {0};

    if (_splitpath_s(path.c_str(), nullptr, 0, nullptr, 0, filename, sizeof(filename), extname, sizeof(extname))) {
        return result;
    }

    result.first = filename;

    if (extname[0] == '.') {
        result.second = &extname[1];
    }

#elif WXBOX_IN_MAC_OS
    char* duplicate = strdup(path.c_str());
    if (!duplicate) {
        return result;
    }

    char* filename = basename(duplicate);
    char* extname  = strrchr(filename, '.');

    if (extname) {
        *extname++    = '\0';
        result.second = extname;
    }

    result.first = filename;

    free(duplicate);
#endif

    return std::move(result);
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

    std::regex matchPattern(R"(^(?:([0-9]+)\.)?(?:([0-9]+)\.)?(?:([0-9]+)\.)?([0-9]+)$)");

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

std::vector<std::string> wxbox::util::file::ListFilesInDirectoryWithExt(const std::string& dirPath, const std::string& ext)
{
#if WXBOX_IN_WINDOWS_OS
    return std::move(ListFilesInDirectoryWithExt_Windows(dirPath, ext));
#elif WXBOX_IN_MAC_OS
    return std::move(ListFilesInDirectoryWithExt_Mac(dirPath, ext));
#endif
}

bool wxbox::util::file::OpenFolderFilesChangeMonitor(const std::string& dirPath, const FileChangeMonitorCallback& callback)
{
#if WXBOX_IN_WINDOWS_OS
    return OpenFolderFilesChangeMonitor_Windows(dirPath, callback);
#elif WXBOX_IN_MAC_OS
    return OpenFolderFilesChangeMonitor_Mac(dirPath, callback);
#endif
}

void wxbox::util::file::CloseFolderFilesChangeMonitor(const std::string& dirPath)
{
    std::lock_guard<std::mutex> lock(g_fileMonitorMutex);

    // cannot monitor the same folder at the same time
    auto pRecord = std::find_if(g_fileMonitorRecords.begin(), g_fileMonitorRecords.end(), [&dirPath](const decltype(g_fileMonitorRecords)::value_type& item) {
        return !::_stricmp(item.first.c_str(), dirPath.c_str());
    });
    if (pRecord == g_fileMonitorRecords.end()) {
        return;
    }

    pRecord->second.closeSignal.set_value();
    pRecord->second.finishFuture.wait_for(std::chrono::milliseconds(FOLDER_FILES_CHANGE_MONITOR_WAIT_FOR_FINISH_TIMEOUT_MS));

    g_fileMonitorRecords.erase(pRecord);
}

void wxbox::util::file::CloseFolderFilesChangeMonitor()
{
    std::lock_guard<std::mutex> lock(g_fileMonitorMutex);

    std::for_each(g_fileMonitorRecords.begin(), g_fileMonitorRecords.end(), [&](decltype(g_fileMonitorRecords)::value_type& pRecord) {
        pRecord.second.closeSignal.set_value();
        pRecord.second.finishFuture.wait_for(std::chrono::milliseconds(FOLDER_FILES_CHANGE_MONITOR_WAIT_FOR_FINISH_TIMEOUT_MS));
    });

    g_fileMonitorRecords.clear();
}

std::time_t wxbox::util::file::GetFileModifyTimestamp(const std::string& path)
{
    if (!IsPathExists(path)) {
        return 0;
    }

    return std::chrono::duration_cast<std::chrono::seconds>(std::experimental::filesystem::last_write_time(path).time_since_epoch()).count();
}