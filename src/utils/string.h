#ifndef __WXBOX_UTILS_STRING_H
#define __WXBOX_UTILS_STRING_H

namespace wxbox {
    namespace util {
        namespace string {

            //
            // Function
            //

            //
            // encoding converter
            //

            // utf8 std::string <-> std::wstring
            std::string  ToUtf8String(const std::wstring& str);
            std::wstring ToUtf8WString(const std::string& str);

            // native std::string <-> std::wstring
            std::string  ToNativeString(const std::wstring& str);
            std::wstring ToNativeWString(const std::string& str);

            // utf8 -> native
            std::string  Utf8ToNativeString(const std::string& str);
            std::wstring Utf8ToNativeWString(const std::wstring& str);

            // native -> utf8
            std::string  NativeToUtf8String(const std::string& str);
            std::wstring NativeToUtf8WString(const std::wstring& str);

            //
            // Split and Join
            //

            std::vector<std::string> SplitString(const std::string& str, const std::string& delim);
            std::string              JoinString(const std::vector<std::string>& vt, const std::string& delim);

            //
            // Trims
            //

            void LTrim(std::string& str);
            void RTrim(std::string& str);
            void Trim(std::string& str);
        }
    }
}

#define IS_MULTI_BYTES_CHAR_CODE(c) ((0x80 & c) != 0)
#define UNWIND_MACRO_STRING_LITERAL(STRING_MACRO) STRING_MACRO

#endif  // #ifndef __WXBOX_UTILS_STRING_H