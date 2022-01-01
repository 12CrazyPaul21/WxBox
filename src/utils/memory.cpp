#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

static inline bool ReadMemory_Windows(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
    SIZE_T numberOfBytesRead = 0;
    bool   retval            = ::MemoryReadSafe(hProcess, const_cast<LPVOID>(pBaseAddress), pBuffer, uSize, &numberOfBytesRead);

    if (pNumberOfBytesRead) {
        *pNumberOfBytesRead = numberOfBytesRead;
    }

    return retval;
}

static inline bool WriteMemory_Windows(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
    SIZE_T numberOfBytesWritten = 0;
    bool   retval               = ::MemoryWriteSafe(hProcess, const_cast<LPVOID>(pBaseAddress), pBuffer, uSize, &numberOfBytesWritten);

    if (pNumberOfBytesWritten) {
        *pNumberOfBytesWritten = numberOfBytesWritten;
    }

    return retval;
}

static inline ucpulong_t ScanMemory_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    if (!hProcess || !pMemBegin || !uMemSize || !pPattern || !uPatternSize) {
        return 0;
    }

    std::vector<uint8_t>     vtBuffer;
    ucpulong_t               pRet               = 0;
    void*                    pReadBuffer        = nullptr;
    uint8_t*                 pSearchBuffer      = nullptr;
    uint8_t*                 pCompareBuffer     = (uint8_t*)const_cast<void*>(pPattern);
    ucpulong_t               uNumberOfBytesRead = 0;
    MEMORY_BASIC_INFORMATION memoryInformation  = {0};

    // load memory, here can optimize using memory mapping!!!!!!
    if (GetCurrentProcess() != hProcess) {
        try {
            vtBuffer.resize(uMemSize);
            pReadBuffer = vtBuffer.data();
        }
        catch (...) {
        }

        if (!pReadBuffer) {
            return 0;
        }

        if (!wb_memory::ReadMemory(hProcess, const_cast<void*>(pMemBegin), (uint8_t*)pReadBuffer, uMemSize, &uNumberOfBytesRead)) {
            if (!::VirtualQueryEx(hProcess, pMemBegin, &memoryInformation, sizeof(memoryInformation))) {
                return 0;
            }

            uMemSize = ((ucpulong_t)memoryInformation.BaseAddress + memoryInformation.RegionSize - (ucpulong_t)pMemBegin);
            if (!wb_memory::ReadMemory(hProcess, const_cast<void*>(pMemBegin), (uint8_t*)pReadBuffer, uMemSize, &uNumberOfBytesRead)) {
                return 0;
            }
        }
        pSearchBuffer = (uint8_t*)pReadBuffer;
    }
    else {
        pSearchBuffer = (uint8_t*)const_cast<void*>(pMemBegin);
    }

    // search
    for (size_t i = 0, j = 0; i < uMemSize && !pRet; i++) {
        for (j = 0; j < uPatternSize; j++) {
            if (pSearchBuffer[i + j] != pCompareBuffer[j]) {
                break;
            }
        }

        if (j == uPatternSize) {
            pRet = (ucpulong_t)pMemBegin + i;
        }
    }

    return pRet;
}

static inline ucpulong_t ScanMemoryRev_Windows(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    if (!hProcess || !pMemRevBegin || !uMemSize || !pPattern || !uPatternSize) {
        return 0;
    }

    // clamp uMemSize
    uint64_t uLastMemSize = std::min<uint64_t>((ucpulong_t)pMemRevBegin, uMemSize);

    // calc actual search range
    LPVOID     pSearchMemBegin = (LPVOID)((ucpulong_t)pMemRevBegin - (ucpulong_t)uLastMemSize);
    LPVOID     pSearchMemEnd   = (LPVOID)pMemRevBegin;
    LPVOID     pSearchCursor   = pSearchMemBegin;
    ucpulong_t lastMatched     = 0;

    // rev scan pattern
    while (uLastMemSize > 0) {
        ucpulong_t matched = wb_memory::ScanMemory(hProcess, pSearchCursor, (ucpulong_t)uLastMemSize, (LPVOID)pPattern, uPatternSize);
        if (!matched) {
            break;
        }

        lastMatched   = matched;
        pSearchCursor = (LPVOID)(matched + 1);
        uLastMemSize  = (uint64_t)pSearchMemEnd - (uint64_t)pSearchCursor;
    }

    return lastMatched;
}

#elif WXBOX_IN_MAC_OS

static inline bool ReadMemory_Mac(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
    return false;
}

static inline bool WriteMemory_Mac(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
    return false;
}

static inline ucpulong_t ScanMemory_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    return 0;
}

static inline ucpulong_t ScanMemoryRev_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    return 0;
}

#endif

bool wxbox::util::memory::ReadMemory(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
#if WXBOX_IN_WINDOWS_OS
    return ReadMemory_Windows(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesRead);
#elif WXBOX_IN_MAC_OS
    return ReadMemory_Mac(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesRead);
#endif
}

bool wxbox::util::memory::WriteMemory(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
#if WXBOX_IN_WINDOWS_OS
    return WriteMemory_Windows(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesWritten);
#elif WXBOX_IN_MAC_OS
    return WriteMemory_Mac(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesWritten);
#endif
}

ucpulong_t wxbox::util::memory::ScanMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
#if WXBOX_IN_WINDOWS_OS
    return ScanMemory_Windows(hProcess, pMemBegin, uMemSize, pPattern, uPatternSize);
#elif WXBOX_IN_MAC_OS
    return ScanMemory_Mac(hProcess, pMemBegin, uMemSize, pPattern, uPatternSize);
#endif
}

ucpulong_t wxbox::util::memory::ScanMemoryRev(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
#if WXBOX_IN_WINDOWS_OS
    return ScanMemoryRev_Windows(hProcess, pMemRevBegin, uMemSize, pPattern, uPatternSize);
#elif WXBOX_IN_MAC_OS
    return ScanMemoryRev_Mac(hProcess, pMemRevBegin, uMemSize, pPattern, uPatternSize);
#endif
}