#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

uint32_t ScanMemory_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
	//
	// just wrap TitanEngine's FindEx
	//

    return ::FindEx(hProcess, (LPVOID)pMemBegin, uMemSize, (LPVOID)pPattern, uPatternSize, nullptr);
}

uint32_t ScanMemoryRev_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
    if (!hProcess || !pMemRevBegin || !uMemSize || !pPattern || !uPatternSize) {
        return 0;
    }

	// clamp uMemSize
    int64_t uLastMemSize = std::min<uint32_t>((uint32_t)pMemRevBegin, uMemSize);

	// calc actual search range
    LPVOID    pSearchMemBegin = (LPVOID)((uint32_t)pMemRevBegin - (uint32_t)uLastMemSize);
    LPVOID    pSearchMemEnd   = (LPVOID)pMemRevBegin;
    LPVOID    pSearchCursor   = pSearchMemBegin;
    ULONG_PTR lastMatched     = 0;

	// rev scan pattern
    while (uLastMemSize > 0) {
        uint32_t matched = wb_memory::ScanMemory(hProcess, pSearchCursor, (uint32_t)uLastMemSize, (LPVOID)pPattern, uPatternSize);
        if (!matched) {
            break;
        }

        lastMatched   = matched;
        pSearchCursor = (LPVOID)(matched + 1);
        uLastMemSize  = (int64_t)pSearchMemEnd - (int64_t)pSearchCursor;
    }

    return lastMatched;
}

#elif WXBOX_IN_MAC_OS

uint32_t ScanMemory_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
    return 0;
}

uint32_t ScanMemoryRev_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
    return 0;
}

#endif

uint32_t wxbox::util::memory::ScanMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
#if WXBOX_IN_WINDOWS_OS
    return ScanMemory_Windows(hProcess, pMemBegin, uMemSize, pPattern, uPatternSize);
#elif WXBOX_IN_MAC_OS
    return ScanMemory_Mac(hProcess, pMemBegin, uMemSize, pPattern, uPatternSize);
#endif
}

uint32_t wxbox::util::memory::ScanMemoryRev(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, uint32_t uMemSize, const void* const pPattern, uint32_t uPatternSize)
{
#if WXBOX_IN_WINDOWS_OS
    return ScanMemoryRev_Windows(hProcess, pMemRevBegin, uMemSize, pPattern, uPatternSize);
#elif WXBOX_IN_MAC_OS
    return ScanMemoryRev_Mac(hProcess, pMemRevBegin, uMemSize, pPattern, uPatternSize);
#endif
}