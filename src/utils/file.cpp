#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

#include <experimental/filesystem>

static inline std::string GetWxBoxRootPath_Windows()
{
    char fullPath[MAX_PATH] = {0};
    char absFullPath[MAX_PATH] = {0};

    if (!::GetModuleFileNameA(NULL, fullPath, MAX_PATH)) {
        return "";
    }

	if (!::GetFullPathNameA(fullPath, MAX_PATH, absFullPath, nullptr)) {
        return "";
	}

	return std::move(std::experimental::filesystem::path(absFullPath).parent_path().string());
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetWxBoxRootPath_Mac()
{
    return "";
}

#endif

std::string wxbox::util::file::GetWxBoxRootPath()
{
    std::string wxboxRootPath = "";

#if WXBOX_PLATFORM == WXBOX_WINDOWS_OS
    wxboxRootPath = GetWxBoxRootPath_Windows();
#elif WXBOX_PLATFORM == WXBOX_MAC_OS
    wxboxRootPath = GetWxBoxRootPath_Mac();
#endif

    return wxboxRootPath;
}