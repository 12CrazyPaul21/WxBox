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
    throw std::exception("ToString_Mac stub");
    return "";
}

static inline std::wstring ToWString_Mac(const std::string& str)
{
    throw std::exception("ToWString_Mac stub");
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

std::vector<std::string> wxbox::util::string::SplitString(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;

#if WXBOX_IN_WINDOWS_OS
    char* tmp = _strdup(str.c_str());
#else
    char* tmp = strdup(str.c_str());
#endif

    if (!tmp) {
        return std::move(result);
    }

    char* token     = nullptr;
    char* nextToken = nullptr;

#if WXBOX_IN_WINDOWS_OS
    for (token = strtok_s(tmp, delim.c_str(), &nextToken); token; token = strtok_s(nullptr, delim.c_str(), &nextToken)) {
#else
    for (token = strtok_r(tmp, delim.c_str(), &nextToken); token; token = strtok_r(nullptr, delim.c_str(), &nextToken)) {
#endif
        result.push_back(token);
    }

    free(tmp);
    return std::move(result);
}

std::string wxbox::util::string::JoinString(const std::vector<std::string>& vt, const std::string& delim)
{
    std::stringstream path;
    for (auto p = vt.begin(); p != vt.end();) {
        path << *p;
        if (++p != vt.end()) {
            path << delim;
        }
    }
    return path.str();
}