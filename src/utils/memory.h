#ifndef __WXBOX_UTILS_MEMORY_H
#define __WXBOX_UTILS_MEMORY_H

namespace wxbox {
    namespace util {
        namespace memory {

            //
            // Function
            //

            bool ReadMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead);
            bool WriteMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten);

            ucpulong_t ScanMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize);
            ucpulong_t ScanMemoryRev(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_MEMORY_H