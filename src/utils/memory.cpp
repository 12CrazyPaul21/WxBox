#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

//
// internal_allocator
//

static std::atomic_size_t g_szInternalAllocatorAllocated = 0;
static HANDLE             g_hInternalAllocatorHeap       = NULL;

static inline bool init_internal_allocator_Windows()
{
    if (g_hInternalAllocatorHeap) {
        return false;
    }

    g_hInternalAllocatorHeap       = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
    g_szInternalAllocatorAllocated = 0;
    return g_hInternalAllocatorHeap != NULL;
}

static inline bool deinit_internal_allocator_Windows()
{
    if (!g_hInternalAllocatorHeap || g_szInternalAllocatorAllocated) {
        return false;
    }

    ::HeapDestroy(g_hInternalAllocatorHeap);
    g_hInternalAllocatorHeap = NULL;
    return true;
}

static inline void* internal_malloc_Windows(size_t n)
{
    if (!n) {
        return nullptr;
    }

    if (!g_hInternalAllocatorHeap && !init_internal_allocator_Windows()) {
        return nullptr;
    }

    auto ptr = ::HeapAlloc(g_hInternalAllocatorHeap, HEAP_ZERO_MEMORY, n);
    if (!ptr) {
        return nullptr;
    }

    g_szInternalAllocatorAllocated += n;
    return ptr;
}

static inline void internal_free_Windows(void* p)
{
    if (!p || !g_hInternalAllocatorHeap) {
        return;
    }

    auto size = ::HeapSize(g_hInternalAllocatorHeap, 0, p);
    ::HeapFree(g_hInternalAllocatorHeap, 0, p);

    if (size != -1) {
        g_szInternalAllocatorAllocated -= size;
        if (g_szInternalAllocatorAllocated <= 0) {
            deinit_internal_allocator_Windows();
        }
    }
}

//
// wxbox::utils::memory
//

static std::mutex         g_mutexAlloc;
static std::atomic_size_t g_szAllocated = 0;
static HANDLE             g_hHeap       = NULL;

static inline void* AllocUnrestrictedMem_Windows(const size_t& count)
{
    if (count == 0) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_mutexAlloc);
    if (!g_hHeap) {
        g_hHeap = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
        if (!g_hHeap) {
            return nullptr;
        }
    }

    void* pMem = ::HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, count);
    if (!pMem) {
        return nullptr;
    }

    g_szAllocated += count;
    return pMem;
}

static inline void FreeUnrestrictedMem_Windows(void* pMem)
{
    if (!pMem) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_mutexAlloc);
    if (!g_hHeap) {
        return;
    }

    auto size = ::HeapSize(g_hHeap, 0, pMem);
    ::HeapFree(g_hHeap, 0, pMem);

    if (size != -1) {
        g_szAllocated -= size;
        if (g_szAllocated <= 0) {
            ::HeapDestroy(g_hHeap);
            g_hHeap = NULL;
        }
    }
}

static inline bool ReadMemory_Windows(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
    SIZE_T numberOfBytesRead = 0;
    bool   retval            = ::ReadProcessMemory(hProcess, pBaseAddress, pBuffer, uSize, &numberOfBytesRead);

    if (pNumberOfBytesRead) {
        *pNumberOfBytesRead = numberOfBytesRead;
    }

    return retval;
}

static inline bool WriteMemory_Windows(wb_process::PROCESS_HANDLE hProcess, void* pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
    SIZE_T numberOfBytesWritten = 0;
    bool   retval               = ::WriteProcessMemory(hProcess, pBaseAddress, pBuffer, uSize, &numberOfBytesWritten);

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

static inline bool init_internal_allocator_Mac()
{
    throw std::exception("init_internal_allocator_Mac stub");
    return false;
}

static inline bool deinit_internal_allocator_Mac()
{
    throw std::exception("deinit_internal_allocator_Mac stub");
    return false;
}

static inline void* internal_malloc_Mac(size_t n)
{
    throw std::exception("internal_malloc_Mac stub");
    return nullptr;
}

static inline void internal_free_Mac(void* p)
{
    throw std::exception("internal_free_Mac stub");
}

static inline void* AllocUnrestrictedMem_Mac(const size_t& count)
{
    throw std::exception("AllocUnrestrictedMem_Mac stub");
    return nullptr;
}

static inline void FreeUnrestrictedMem_Mac(void* pMem)
{
    throw std::exception("FreeUnrestrictedMem_Mac stub");
}

static inline bool ReadMemory_Mac(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
    throw std::exception("ReadMemory_Mac stub");
    return false;
}

static inline bool WriteMemory_Mac(wb_process::PROCESS_HANDLE hProcess, void* pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
    throw std::exception("WriteMemory_Mac stub");
    return false;
}

static inline ucpulong_t ScanMemory_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    throw std::exception("ScanMemory_Mac stub");
    return 0;
}

static inline ucpulong_t ScanMemoryRev_Mac(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize)
{
    throw std::exception("ScanMemoryRev_Mac stub");
    return 0;
}

#endif

//
// internal_allocator
//

bool wxbox::util::memory::init_internal_allocator()
{
#if WXBOX_IN_WINDOWS_OS
    return init_internal_allocator_Windows();
#elif WXBOX_IN_MAC_OS
    return init_internal_allocator_Mac();
#endif
}

bool wxbox::util::memory::deinit_internal_allocator()
{
#if WXBOX_IN_WINDOWS_OS
    return deinit_internal_allocator_Windows();
#elif WXBOX_IN_MAC_OS
    return deinit_internal_allocator_Mac();
#endif
}

void* wxbox::util::memory::internal_malloc(size_t n)
{
#if WXBOX_IN_WINDOWS_OS
    return internal_malloc_Windows(n);
#elif WXBOX_IN_MAC_OS
    return internal_malloc_Mac(n);
#endif
}

void wxbox::util::memory::internal_free(void* p)
{
#if WXBOX_IN_WINDOWS_OS
    internal_free_Windows(p);
#elif WXBOX_IN_MAC_OS
    internal_free_Mac(p);
#endif
}

//
// wxbox::utils::memory
//

void* wxbox::util::memory::AllocUnrestrictedMem(const size_t& count)
{
#if WXBOX_IN_WINDOWS_OS
    return AllocUnrestrictedMem_Windows(count);
#elif WXBOX_IN_MAC_OS
    return AllocUnrestrictedMem_Mac(count);
#endif
}

void wxbox::util::memory::FreeUnrestrictedMem(void* pMem)
{
#if WXBOX_IN_WINDOWS_OS
    return FreeUnrestrictedMem_Windows(pMem);
#elif WXBOX_IN_MAC_OS
    return FreeUnrestrictedMem_Mac(pMem);
#endif
}

wxbox::util::memory::RemotePageInfo wxbox::util::memory::AllocPageToRemoteProcess(wxbox::util::process::PROCESS_HANDLE hProcess, ucpulong_t pageSize, bool canExecute)
{
    // at least one page
    pageSize = std::max<ucpulong_t>(pageSize, RemotePageInfo::MIN_REMOTE_PAGE_SIZE);

    RemotePageInfo pageInfo;
    std::memset(&pageInfo, 0, sizeof(pageInfo));

    if (!hProcess) {
        return pageInfo;
    }

    LPVOID lpAddress = nullptr;

#if WXBOX_IN_WINDOWS_OS
    lpAddress = VirtualAllocEx(hProcess, nullptr, pageSize, MEM_COMMIT | MEM_RESERVE, canExecute ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
#else
    throw std::exception("AllocPageToRemoteProcess stub");
#endif

    if (!lpAddress) {
        return pageInfo;
    }

    pageInfo.addr   = lpAddress;
    pageInfo.size   = pageSize;
    pageInfo.cursor = pageInfo.addr;
    pageInfo.end    = (void*)((ucpulong_t)pageInfo.addr + pageInfo.size);
    pageInfo.free   = pageSize;

    return pageInfo;
}

bool wxbox::util::memory::FreeRemoteProcessPage(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo)
{
    if (!pageInfo.addr) {
        return false;
    }

    void*      addr = pageInfo.addr;
    ucpulong_t size = pageInfo.size;

    pageInfo.addr   = nullptr;
    pageInfo.size   = 0;
    pageInfo.cursor = nullptr;
    pageInfo.end    = nullptr;
    pageInfo.free   = 0;

#if WXBOX_IN_WINDOWS_OS
    return VirtualFreeEx(hProcess, addr, size, MEM_DECOMMIT);
#else
    throw std::exception("FreeRemoteProcessPage stub");
    return false;
#endif
}

bool wxbox::util::memory::ReadMemory(wb_process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead)
{
#if WXBOX_IN_WINDOWS_OS
    return ReadMemory_Windows(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesRead);
#elif WXBOX_IN_MAC_OS
    return ReadMemory_Mac(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesRead);
#endif
}

bool wxbox::util::memory::WriteMemory(wb_process::PROCESS_HANDLE hProcess, void* pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten)
{
#if WXBOX_IN_WINDOWS_OS
    return WriteMemory_Windows(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesWritten);
#elif WXBOX_IN_MAC_OS
    return WriteMemory_Mac(hProcess, pBaseAddress, pBuffer, uSize, pNumberOfBytesWritten);
#endif
}

wxbox::util::memory::RemoteWrittenMemoryInfo wxbox::util::memory::WriteByteStreamToProcess(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo, const uint8_t* const byteStream, ucpulong_t size)
{
    RemoteWrittenMemoryInfo memInfo;

    if (!hProcess || !pageInfo.addr || pageInfo.free < size || !byteStream || !size) {
        return memInfo;
    }

    LPVOID lpAddress            = pageInfo.cursor;
    DWORD  numberOfBytesWritten = 0;

    if (!wb_memory::WriteMemory(hProcess, lpAddress, byteStream, size, &numberOfBytesWritten)) {
        VirtualFreeEx(hProcess, lpAddress, size, MEM_DECOMMIT);
        return memInfo;
    }

    memInfo.addr = lpAddress;
    memInfo.size = size;

    pageInfo.cursor = (void*)((ucpulong_t)memInfo.addr + memInfo.size);
    pageInfo.free   = pageInfo.free - memInfo.size;

    return memInfo;
}

wxbox::util::memory::RemoteWrittenMemoryInfo wxbox::util::memory::WriteStringToProcess(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo, const std::string& str)
{
    if (!hProcess || !pageInfo.addr || !pageInfo.free || str.empty()) {
        return RemoteWrittenMemoryInfo();
    }

    return WriteByteStreamToProcess(hProcess, pageInfo, reinterpret_cast<const uint8_t* const>(str.c_str()), str.length() + 1);
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