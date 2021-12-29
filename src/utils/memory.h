#ifndef __WXBOX_UTILS_MEMORY_H
#define __WXBOX_UTILS_MEMORY_H

namespace wxbox {
    namespace util {
		namespace memory {

			//
			// Function
			//

			uint32_t ScanMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize);
            uint32_t ScanMemoryRev(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize);
		}
    }
}

#endif  // #ifndef __WXBOX_UTILS_MEMORY_H