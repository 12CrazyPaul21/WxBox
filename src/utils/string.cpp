#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

static inline std::string ToString_Windows(const std::wstring& str, int codepage = CP_UTF8)
{
    int len = ::WideCharToMultiByte(codepage, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return "";
    }

    std::unique_ptr<char[]> buffer(new char[len]);
    if (buffer == nullptr) {
        return "";
    }

    ::WideCharToMultiByte(codepage, 0, str.c_str(), -1, buffer.get(), len, nullptr, nullptr);
    return std::string(buffer.get());
}

static inline std::wstring ToWString_Windows(const std::string& str, int codepage = CP_UTF8)
{
    int len = ::MultiByteToWideChar(codepage, 0, str.c_str(), -1, nullptr, 0);
    if (len == 0) {
        return L"";
    }

    std::unique_ptr<wchar_t[]> buffer(new wchar_t[len]);
    if (buffer == nullptr) {
        return L"";
    }

    ::MultiByteToWideChar(codepage, 0, str.c_str(), -1, buffer.get(), len);
    return std::wstring(buffer.get());
}

#elif WXBOX_IN_MAC_OS

static inline std::string ToUtf8String_Mac(const std::wstring& str)
{
    throw std::exception("ToUtf8String_Mac stub");
    return "";
}

static inline std::wstring ToUtf8WString_Mac(const std::string& str)
{
    throw std::exception("ToUtf8WString_Mac stub");
    return L"";
}

static inline std::string ToNativeString_Mac(const std::wstring& str)
{
    throw std::exception("ToNativeString_Mac stub");
    return "";
}

static inline std::wstring ToNativeWString_Mac(const std::string& str)
{
    throw std::exception("ToNativeWString_Mac stub");
    return L"";
}

static inline std::string Utf8ToNativeString_Mac(const std::string& str)
{
    throw std::exception("Utf8ToNativeString_Mac stub");
    return "";
}

static inline std::wstring Utf8ToNativeWString_Mac(const std::wstring& str)
{
    throw std::exception("Utf8ToNativeWString_Mac stub");
    return L"";
}

static inline std::string NativeToUtf8String_Mac(const std::string& str)
{
    throw std::exception("NativeToUtf8String_Mac stub");
    return "";
}

static inline std::wstring NativeToUtf8WString_Mac(const std::wstring& str)
{
    throw std::exception("NativeToUtf8WString_Mac stub");
    return L"";
}

#endif

//
// utf8 std::string <-> std::wstring
//

std::string wxbox::util::string::ToUtf8String(const std::wstring& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToString_Windows(str, CP_UTF8);
#elif WXBOX_IN_MAC_OS
    return ToUtf8String_Mac(str);
#endif
}

std::wstring wxbox::util::string::ToUtf8WString(const std::string& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToWString_Windows(str, CP_UTF8);
#elif WXBOX_IN_MAC_OS
    return ToUtf8WString_Mac(str);
#endif
}

//
// native std::string <-> std::wstring
//

std::string wxbox::util::string::ToNativeString(const std::wstring& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToString_Windows(str, CP_ACP);
#elif WXBOX_IN_MAC_OS
    return ToNativeString_Mac(str);
#endif
}

std::wstring wxbox::util::string::ToNativeWString(const std::string& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToWString_Windows(str, CP_ACP);
#elif WXBOX_IN_MAC_OS
    return ToNativeWString_Mac(str);
#endif
}

//
// utf8 -> native
//

std::string wxbox::util::string::Utf8ToNativeString(const std::string& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToNativeString(ToUtf8WString(str));
#elif WXBOX_IN_MAC_OS
    return Utf8ToNativeString_Mac(str);
#endif
}

std::wstring wxbox::util::string::Utf8ToNativeWString(const std::wstring& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToNativeWString(ToUtf8String(str));
#elif WXBOX_IN_MAC_OS
    return Utf8ToNativeWString_Mac(str);
#endif
}

//
// native -> utf8
//

std::string wxbox::util::string::NativeToUtf8String(const std::string& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToUtf8String(ToNativeWString(str));
#elif WXBOX_IN_MAC_OS
    return NativeToUtf8String_Mac(str);
#endif
}

std::wstring wxbox::util::string::NativeToUtf8WString(const std::wstring& str)
{
#if WXBOX_IN_WINDOWS_OS
    return ToUtf8WString(ToNativeString(str));
#elif WXBOX_IN_MAC_OS
    return NativeToUtf8WString_Mac(str);
#endif
}

//
// Split and Join
//

std::vector<std::string> wxbox::util::string::SplitString(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;

#if WXBOX_IN_WINDOWS_OS
    char* tmp = _strdup(str.c_str());
#else
    char* tmp = strdup(str.c_str());
#endif

    if (!tmp) {
        return result;
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
    return result;
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

//
// Trims
//

void wxbox::util::string::LTrim(std::string& str)
{
    str.erase(str.begin(),
              std::find_if(str.begin(), str.end(), [](unsigned char ch) {
                  return !isspace(ch);
              }));
}

void wxbox::util::string::RTrim(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
                  return !isspace(ch);
              }).base(),
              str.end());
}

void wxbox::util::string::Trim(std::string& str)
{
    LTrim(str);
    RTrim(str);
}