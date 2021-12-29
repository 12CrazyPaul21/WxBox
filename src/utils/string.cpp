#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

static inline std::string ToString_Windows(const std::wstring& str)
{
    int len = ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return "";
    }

    std::unique_ptr<char[]> buffer(new char[len]);
    if (buffer == nullptr) {
        return "";
    }

    ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer.get(), len, nullptr, nullptr);
    return std::move(std::string(buffer.get()));
}

static inline std::wstring ToWString_Windows(const std::string& str)
{
    int len = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len == 0) {
        return L"";
    }

    std::unique_ptr<wchar_t[]> buffer(new wchar_t[len]);
    if (buffer == nullptr) {
        return L"";
    }

    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.get(), len);
    return std::move(std::wstring(buffer.get()));
}

#elif WXBOX_IN_MAC_OS

static inline std::string ToString_Mac(const std::wstring& str)
{
    return "";
}

static inline std::wstring ToWString_Mac(const std::string& str)
{
    return L"";
}

#endif

std::string wxbox::util::string::ToString(const std::wstring& str)
{
	//
    // utf8 wstring to utf8 string
	//

#if WXBOX_IN_WINDOWS_OS
    return std::move(ToString_Windows(str));
#elif WXBOX_IN_MAC_OS
    return std::move(ToString_Mac(str));
#endif
}

std::wstring wxbox::util::string::ToWString(const std::string& str)
{
	//
    // utf8 string to utf8 wstring
	//

#if WXBOX_IN_WINDOWS_OS
    return std::move(ToWString_Windows(str));
#elif WXBOX_IN_MAC_OS
    return std::move(ToWString_Mac(str));
#endif
}