#include <utils/common.h>

// Global Variables
static std::unordered_map<void*, std::vector<uint8_t>> g_vtOriginalEntryBackup;
static std::mutex                                      g_mutexHook;

#if WXBOX_IN_WINDOWS_OS

#if WXBOX_CPU_IS_X86

static bool InProcessHook_Windows_x86(void* pfnOriginal, void* pfnNewEntry)
{
    if (!pfnOriginal || !pfnNewEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtOriginalEntryBackup.find(pfnOriginal) != g_vtOriginalEntryBackup.end()) {
        return false;
    }

    // jmp far op code : jmp(0xEP) <4 bytes - relative address>
    DWORD dwJmpFarInstructionSize = 1 + sizeof(void*);

    // calc relative address for jmp far
    DWORD dwRelativeAddress = (DWORD)pfnNewEntry - ((DWORD)pfnOriginal + dwJmpFarInstructionSize);

    // backup original entry
    std::vector<uint8_t> originalEntry;
    for (size_t i = 0; i < dwJmpFarInstructionSize; i++) {
        originalEntry.push_back(((uint8_t*)pfnOriginal)[i]);
    }

    //
    // replace original function entry
    //

    uint8_t newEntry[5];

    // jmp far op code
    newEntry[0] = 0xE9;

    // relative address
    *((DWORD*)(&newEntry[1])) = dwRelativeAddress;

    // write memory
    ucpulong_t szNumberOfBytesWritten = 0;
    if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, newEntry, dwJmpFarInstructionSize, &szNumberOfBytesWritten)) {
        return false;
    }

    // record original entry backup
    g_vtOriginalEntryBackup[pfnOriginal] = std::move(originalEntry);

    return true;
}

#else

static bool InProcessHook_Windows_x64(void* pfnOriginal, void* pfnNewEntry)
{
    if (!pfnOriginal || !pfnNewEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtOriginalEntryBackup.find(pfnOriginal) != g_vtOriginalEntryBackup.end()) {
        return false;
    }

    //
    // way
    //

    // push low-32-bit
    // mov dword ptr ss:[rsp+4], high-32-bit
    // ret

    //
    // instruction sketch[total 14 bytes]
    //

    // 0x68 [low-32-bit]
    // 0xC7 0x44 0x24 0x04 [high-32-bit]
    // 0xC3

    //
    // fill replace entry code
    //

    uint8_t shellcode[] = {
        // push low-32-bit
        0x68,
        0xCC,
        0xCC,
        0xCC,
        0xCC,
        // mov dword ptr ss:[rsp+4], high-32-bit
        0xC7,
        0x44,
        0x24,
        0x04,
        0xCC,
        0xCC,
        0xCC,
        0xCC,
        // ret
        0xC3};

    *((uint32_t*)&shellcode[1]) = (uint64_t)pfnNewEntry & 0xFFFFFFFF;
    *((uint32_t*)&shellcode[9]) = (((uint64_t)pfnNewEntry) >> 32) & 0xFFFFFFFF;

    // backup original entry
    std::vector<uint8_t> originalEntry;
    for (size_t i = 0; i < sizeof(shellcode); i++) {
        originalEntry.push_back(((uint8_t*)pfnOriginal)[i]);
    }

    // replace original function entry
    ucpulong_t szNumberOfBytesWritten = 0;
    if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, shellcode, sizeof(shellcode), &szNumberOfBytesWritten)) {
        return false;
    }

    // record original entry backup
    g_vtOriginalEntryBackup[pfnOriginal] = std::move(originalEntry);

    return true;
}

#endif

static bool InProcessHook_Windows(void* pfnOriginal, void* pfnNewEntry)
{
#if WXBOX_CPU_IS_X86
    return InProcessHook_Windows_x86(pfnOriginal, pfnNewEntry);
#else
    return InProcessHook_Windows_x64(pfnOriginal, pfnNewEntry);
#endif
}

#elif WXBOX_IN_MAC_OS

static bool InProcessHook_Mac(void* pfnOriginal, void* pfnNewEntry)
{
    throw std::exception("InProcessHook_Mac stub");
    return false;
}

#endif

bool wxbox::util::hook::InProcessHook(void* pfnOriginal, void* pfnNewEntry)
{
#if WXBOX_IN_WINDOWS_OS
    return InProcessHook_Windows(pfnOriginal, pfnNewEntry);
#elif WXBOX_IN_MAC_OS
    return InProcessHook_Mac(pfnOriginal, pfnNewEntry);
#endif
}

bool wxbox::util::hook::InProcessDummyHook(void* pfnOriginal, void* pfnDummy)
{
    return InProcessHook(pfnOriginal, pfnDummy);
}

bool wxbox::util::hook::RevokeInProcessHook(void* pfnEntry)
{
    if (!pfnEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    auto                        it = g_vtOriginalEntryBackup.find(pfnEntry);
    if (it == g_vtOriginalEntryBackup.end()) {
        return false;
    }

    auto&      backup                 = it->second;
    ucpulong_t szNumberOfBytesWritten = 0;
    if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnEntry, backup.data(), backup.size(), &szNumberOfBytesWritten)) {
        return false;
    }

    g_vtOriginalEntryBackup.erase(it);
    return true;
}