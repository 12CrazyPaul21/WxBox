#ifndef __WXBOX_UTILS_STRING_H
#define __WXBOX_UTILS_STRING_H

namespace wxbox {
    namespace util {
        namespace string {

            //
            // Function
            //

            std::string  ToString(const std::wstring& str);
            std::wstring ToWString(const std::string& str);

            std::vector<std::string> SplitString(const std::string& str, const std::string& delim);
            std::string              JoinString(const std::vector<std::string>& vt, const std::string& delim);

            void LTrim(std::string& str);
            void RTrim(std::string& str);
            void Trim(std::string& str);
        }
    }
}

#define IS_MULTI_BYTES_CHAR_CODE(c) ((0x80 & c) != 0)
#define UNWIND_MACRO_STRING_LITERAL(STRING_MACRO) STRING_MACRO

#endif  // #ifndef __WXBOX_UTILS_STRING_H