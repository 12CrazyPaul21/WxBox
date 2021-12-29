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

#endif  // #ifndef __WXBOX_UTILS_STRING_H