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
        }
    }
}

#define IS_MULTI_BYTES_CHAR_CODE(c) ((0x80 & c) != 0)

#endif  // #ifndef __WXBOX_UTILS_STRING_H