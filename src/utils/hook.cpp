#include <utils/common.h>

// Global Variables
static std::unordered_map<void*, wb_hook::HookMetaInfo> g_vtHookRecord;
static std::mutex                                       g_mutexHook;

#if WXBOX_IN_WINDOWS_OS

#if WXBOX_CPU_IS_X86

// jmp far op code : jmp(0xEP) <4 bytes - relative address>
static constexpr DWORD dwJmpFarInstructionSize = 1 + sizeof(void*);

static bool Do_InProcessHook_Windows_x86(void* pfnOriginal, void* pfnNewEntry, std::vector<uint8_t>& hookOpcodes, std::vector<uint8_t>& originalEntryBackup, bool isPre = false)
{
    // calc relative address for jmp far
    DWORD dwRelativeAddress = (DWORD)pfnNewEntry - ((DWORD)pfnOriginal + dwJmpFarInstructionSize);

    // backup original entry
    originalEntryBackup.clear();
    for (size_t i = 0; i < dwJmpFarInstructionSize; i++) {
        originalEntryBackup.push_back(((uint8_t*)pfnOriginal)[i]);
    }

    //
    // replace original function entry
    //

    uint8_t newEntry[5];

    // jmp far op code
    newEntry[0] = 0xE9;

    // relative address
    *((DWORD*)(&newEntry[1])) = dwRelativeAddress;

    // backup
    hookOpcodes.clear();
    hookOpcodes.insert(hookOpcodes.begin(), &newEntry[0], (&newEntry[0]) + sizeof(newEntry));

    // change protect attribute
    DWORD oldProtect = 0;
    if (!VirtualProtect(pfnOriginal, sizeof(newEntry), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    // write memory
    if (!isPre) {
        ucpulong_t szNumberOfBytesWritten = 0;
        if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, newEntry, dwJmpFarInstructionSize, &szNumberOfBytesWritten)) {
            return false;
        }
    }

    return true;
}

static bool InProcessHook_Windows_x86(void* pfnOriginal, void* pfnNewEntry)
{
    if (!pfnOriginal || !pfnNewEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtHookRecord.find(pfnOriginal) != g_vtHookRecord.end()) {
        return false;
    }

    // execute hook
    wb_hook::HookMetaInfo hookMetaInfo;
    if (!Do_InProcessHook_Windows_x86(pfnOriginal, pfnNewEntry, hookMetaInfo.hookOpcodes, hookMetaInfo.originalEntryBackup, false)) {
        return false;
    }

    hookMetaInfo.type           = wb_hook::HookPointType::Hook;
    hookMetaInfo.entry          = pfnOriginal;
    hookMetaInfo.actualEntry    = pfnOriginal;
    hookMetaInfo.repeater       = nullptr;
    hookMetaInfo.isPre          = false;
    hookMetaInfo.delayedRelease = false;
    g_vtHookRecord.emplace(pfnOriginal, std::move(hookMetaInfo));
    return true;
}

static bool Do_InProcessRelocateIntercept_Windows_x86(void* pfnOriginal, void* pfnStubEntry, bool isPre = false)
{
    // get function's actual entry address
    void* actualEntry = wb_traits::GetActualEntryAddress(pfnOriginal);
    if (!actualEntry) {
        return false;
    }

    // allocate memory for repeater
    size_t   repeaterSize = 24;
    uint8_t* repeater     = (uint8_t*)wb_memory::AllocUnrestrictedMem(repeaterSize);
    if (!repeater) {
        return false;
    }

    // record stub and original absolute address
    *((unsigned long*)&repeater[0]) = (unsigned long)pfnStubEntry;
    *((unsigned long*)&repeater[4]) = (unsigned long)actualEntry;

    //
    // pushad : 0x60
    // pushfd : 0x9C
    //

    repeater[8] = 0x60;
    repeater[9] = 0x9C;

    //
    // call dword ptr to stub  :
    //  memory indirect ModR/M : [disp32]
    //                     mod : 00
    //           extend opcode : 010
    //                     r/m : 101
    //

    repeater[10]                     = 0xFF;
    repeater[11]                     = 0x15;
    *((unsigned long*)&repeater[12]) = (unsigned long)(&repeater[0]);

    //
    // popfd : 0x9D
    // popad : 0x61
    //

    repeater[16] = 0x9D;
    repeater[17] = 0x61;

    //
    // jmp dword ptr to original entry :
    //          memory indirect ModR/M : [disp32]
    //                             mod : 00
    //                   extend opcode : 100
    //                             r/m : 101
    //

    repeater[18]                     = 0xFF;
    repeater[19]                     = 0x25;
    *((unsigned long*)&repeater[20]) = (unsigned long)(&repeater[4]);

    //
    // execute hook
    //

    wb_hook::HookMetaInfo hookMetaInfo;
    if (!Do_InProcessHook_Windows_x86(pfnOriginal, &repeater[8], hookMetaInfo.hookOpcodes, hookMetaInfo.originalEntryBackup, isPre)) {
        wb_memory::FreeUnrestrictedMem(repeater);
        return false;
    }

    hookMetaInfo.type           = wb_hook::HookPointType::RelocateIntercept;
    hookMetaInfo.entry          = pfnOriginal;
    hookMetaInfo.actualEntry    = actualEntry;
    hookMetaInfo.repeater       = repeater;
    hookMetaInfo.repeaterSize   = repeaterSize;
    hookMetaInfo.isPre          = isPre;
    hookMetaInfo.delayedRelease = isPre;
    g_vtHookRecord.emplace(pfnOriginal, std::move(hookMetaInfo));
    return true;
}

// warning: this operation is unsafe
static bool Do_InProcessIntercept_Windows_x86(void* pfnOriginal, void* pfnStubEntry, bool isPre = false)
{
    //
    // calc valid entry opcode serial length
    //

    size_t   opcodeSerialLength    = 0;
    size_t   minLength             = dwJmpFarInstructionSize;
    uint8_t* pOriginalOpcodeCursor = reinterpret_cast<uint8_t*>(pfnOriginal);

    while (opcodeSerialLength < minLength) {
        size_t opcodeLength = wb_traits::CpuOpcodeInstructionByteSize(pOriginalOpcodeCursor);
        opcodeSerialLength += opcodeLength;
        pOriginalOpcodeCursor += opcodeLength;
    }

    //
    // construct repeater
    //

    // allocate memory for repeater
    size_t   repeaterSize = 24 + opcodeSerialLength;
    uint8_t* repeater     = (uint8_t*)wb_memory::AllocUnrestrictedMem(repeaterSize);
    if (!repeater) {
        return false;
    }

    // record stub and original absolute address
    *((unsigned long*)&repeater[0]) = (unsigned long)pfnStubEntry;
    *((unsigned long*)&repeater[4]) = ((unsigned long)pfnOriginal) + opcodeSerialLength;

    //
    // pushad : 0x60
    // pushfd : 0x9C
    //

    repeater[8] = 0x60;
    repeater[9] = 0x9C;

    //
    // call dword ptr to stub  :
    //  memory indirect ModR/M : [disp32]
    //                     mod : 00
    //           extend opcode : 010
    //                     r/m : 101
    //

    repeater[10]                     = 0xFF;
    repeater[11]                     = 0x15;
    *((unsigned long*)&repeater[12]) = (unsigned long)(&repeater[0]);

    //
    // popfd : 0x9D
    // popad : 0x61
    //

    repeater[16] = 0x9D;
    repeater[17] = 0x61;

    //
    // read original valid opcode serials
    //

    ucpulong_t szNumberOfBytesRead = 0;
    if (!wb_memory::ReadMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, &repeater[18], opcodeSerialLength, &szNumberOfBytesRead)) {
        wb_memory::FreeUnrestrictedMem(repeater);
        return false;
    }

    //
    // jmp dword ptr to original entry :
    //          memory indirect ModR/M : [disp32]
    //                             mod : 00
    //                   extend opcode : 100
    //                             r/m : 101
    //

    repeater[18 + opcodeSerialLength]                     = 0xFF;
    repeater[19 + opcodeSerialLength]                     = 0x25;
    *((unsigned long*)&repeater[20 + opcodeSerialLength]) = (unsigned long)(&repeater[4]);

    //
    // execute hook
    //

    wb_hook::HookMetaInfo hookMetaInfo;
    if (!Do_InProcessHook_Windows_x86(pfnOriginal, &repeater[8], hookMetaInfo.hookOpcodes, hookMetaInfo.originalEntryBackup, isPre)) {
        wb_memory::FreeUnrestrictedMem(repeater);
        return false;
    }

    hookMetaInfo.type           = wb_hook::HookPointType::Intercept;
    hookMetaInfo.entry          = pfnOriginal;
    hookMetaInfo.actualEntry    = pfnOriginal;
    hookMetaInfo.repeater       = repeater;
    hookMetaInfo.repeaterSize   = repeaterSize;
    hookMetaInfo.isPre          = isPre;
    hookMetaInfo.delayedRelease = isPre;
    g_vtHookRecord.emplace(pfnOriginal, std::move(hookMetaInfo));
    return true;
}

static bool InProcessIntercept_Windows_x86(void* pfnOriginal, void* pfnStubEntry)
{
    if (!pfnOriginal || !pfnStubEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtHookRecord.find(pfnOriginal) != g_vtHookRecord.end()) {
        return false;
    }

    return (((uint8_t*)pfnOriginal)[0] != 0xE9) ? Do_InProcessIntercept_Windows_x86(pfnOriginal, pfnStubEntry)
                                                : Do_InProcessRelocateIntercept_Windows_x86(pfnOriginal, pfnStubEntry);
}

static bool PreInProcessIntercept_Windows_x86(void* pfnOriginal, void* pfnStubEntry)
{
    if (!pfnOriginal || !pfnStubEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtHookRecord.find(pfnOriginal) != g_vtHookRecord.end()) {
        return false;
    }

    return (((uint8_t*)pfnOriginal)[0] != 0xE9) ? Do_InProcessIntercept_Windows_x86(pfnOriginal, pfnStubEntry, true)
                                                : Do_InProcessRelocateIntercept_Windows_x86(pfnOriginal, pfnStubEntry, true);
}

#else

static bool InProcessHook_Windows_x64(void* pfnOriginal, void* pfnNewEntry)
{
    if (!pfnOriginal || !pfnNewEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    if (g_vtHookRecord.find(pfnOriginal) != g_vtHookRecord.end()) {
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

    // change protect attribute
    DWORD oldProtect = 0;
    if (!VirtualProtect(pfnOriginal, sizeof(newEntry), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    // replace original function entry
    ucpulong_t szNumberOfBytesWritten = 0;
    if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, shellcode, sizeof(shellcode), &szNumberOfBytesWritten)) {
        return false;
    }

    // record original entry backup
    wb_hook::HookMetaInfo hookMetaInfo;
    hookMetaInfo.type                = wb_hook::HookPointType::Hook;
    hookMetaInfo.entry               = pfnOriginal;
    hookMetaInfo.actualEntry         = pfnOriginal;
    hookMetaInfo.repeater            = nullptr;
    hookMetaInfo.originalEntryBackup = std::move(originalEntry);
    g_vtHookRecord.emplace(pfnOriginal, std::move(hookMetaInfo));

    return true;
}

static bool InProcessIntercept_Windows_x64(void* pfnOriginal, void* pfnStubEntry)
{
    throw std::exception("InProcessIntercept_Windows_x64 stub");
    return false;
}

static bool PreInProcessIntercept_Windows_x64(void* pfnOriginal, void* pfnStubEntry)
{
    throw std::exception("PreInProcessIntercept_Windows_x64 stub");
    return false;
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

static bool InProcessIntercept_Windows(void* pfnOriginal, void* pfnStubEntry)
{
#if WXBOX_CPU_IS_X86
    return InProcessIntercept_Windows_x86(pfnOriginal, pfnStubEntry);
#else
    return InProcessIntercept_Windows_x64(pfnOriginal, pfnStubEntry);
#endif
}

static bool PreInProcessIntercept_Windows(void* pfnOriginal, void* pfnStubEntry)
{
#if WXBOX_CPU_IS_X86
    return PreInProcessIntercept_Windows_x86(pfnOriginal, pfnStubEntry);
#else
    return PreInProcessIntercept_Windows_x64(pfnOriginal, pfnStubEntry);
#endif
}

#elif WXBOX_IN_MAC_OS

static bool InProcessHook_Mac(void* pfnOriginal, void* pfnNewEntry)
{
    throw std::exception("InProcessHook_Mac stub");
    return false;
}

static bool InProcessIntercept_Mac(void* pfnOriginal, void* pfnStubEntry)
{
    throw std::exception("InProcessBridgeHook_Mac stub");
    return false;
}

static bool PreInProcessIntercept_Mac(void* pfnOriginal, void* pfnStubEntry)
{
    throw std::exception("PreInProcessBridgeHook_Mac stub");
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

/**
 *
 * Stub Example:
 *
 *    BEGIN_NAKED_STD_FUNCTION(relocate_intercept_hook_stub)
 *    {
 *        __asm {
 *		      push eax
 *		      // get original entry esp
 *		      mov eax, [esp+WXBOX_INTERCEPT_STUB_ORIGINAL_ESP_OFFSET+4]
 *		      push [eax+4]
 *		      call after_relocate_intercept_hook
 *		      add esp, 4
 *		      pop eax
 *		      ret
 *        }
 *    }
 *    END_NAKED_STD_FUNCTION(relocate_intercept_hook_stub)
 *
 * Stub Entry Stack Frame Structure:
 * 
 *    Ret Address      : [ESP](Stack Top)
 *    Original EFLAGS  : [ESP+0x04]
 *    Original EDI     : [ESP+0x08]
 *    Original ESI     : [ESP+0x0C]
 *    Original EBP     : [ESP+0x10]
 *    Original ESP     : [ESP+0x14]
 *    Original EBX     : [ESP+0x18]
 *    Original EDX     : [ESP+0x1C]
 *    Original ECX     : [ESP+0x20]
 *    Original EAX     : [ESP+0x24]
 */
bool wxbox::util::hook::InProcessIntercept(void* pfnOriginal, void* pfnStubEntry)
{
#if WXBOX_IN_WINDOWS_OS
    return InProcessIntercept_Windows(pfnOriginal, pfnStubEntry);
#elif WXBOX_IN_MAC_OS
    return InProcessIntercept_Mac(pfnOriginal, pfnStubEntry);
#endif
}

bool wxbox::util::hook::PreInProcessIntercept(void* pfnOriginal, void* pfnStubEntry)
{
#if WXBOX_IN_WINDOWS_OS
    return PreInProcessIntercept_Windows(pfnOriginal, pfnStubEntry);
#elif WXBOX_IN_MAC_OS
    return PreInProcessIntercept_Mac(pfnOriginal, pfnStubEntry);
#endif
}

bool wxbox::util::hook::ExecuteInProcessIntercept(void* pfnOriginal)
{
    if (!pfnOriginal) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    auto                        it = g_vtHookRecord.find(pfnOriginal);
    if (it == g_vtHookRecord.end()) {
        return false;
    }

    HookMetaInfo& meta = it->second;
    if (!meta.isPre || meta.hookOpcodes.empty()) {
        return false;
    }

    ucpulong_t szNumberOfBytesWritten = 0;
    if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), pfnOriginal, meta.hookOpcodes.data(), meta.hookOpcodes.size(), &szNumberOfBytesWritten)) {
        return false;
    }

    meta.isPre = false;
    return true;
}

bool wxbox::util::hook::ReleasePreInProcessInterceptItem(void* pfnOriginal)
{
    if (!pfnOriginal) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    auto                        it = g_vtHookRecord.find(pfnOriginal);
    if (it == g_vtHookRecord.end()) {
        return false;
    }

    HookMetaInfo& meta = it->second;
    if (!meta.delayedRelease) {
        return false;
    }

    if (meta.repeater) {
        wb_memory::FreeUnrestrictedMem(meta.repeater);
        meta.repeater = nullptr;
    }

    g_vtHookRecord.erase(it);
    return true;
}

bool wxbox::util::hook::RevokeInProcessHook(void* pfnEntry)
{
    if (!pfnEntry) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    auto                        it = g_vtHookRecord.find(pfnEntry);
    if (it == g_vtHookRecord.end()) {
        return false;
    }

    HookMetaInfo& meta = it->second;
    if (meta.repeater && !meta.delayedRelease) {
        wb_memory::FreeUnrestrictedMem(meta.repeater);
        meta.repeater = nullptr;
    }

    if (!meta.isPre) {
        ucpulong_t szNumberOfBytesWritten = 0;
        if (!wb_memory::WriteMemory(wb_process::GetCurrentProcessHandle(), meta.entry, meta.originalEntryBackup.data(), meta.originalEntryBackup.size(), &szNumberOfBytesWritten)) {
            return false;
        }
    }

    if (!meta.delayedRelease) {
        g_vtHookRecord.erase(it);
    }
    return true;
}

bool wxbox::util::hook::ObtainHookMetaInfo(void* pfnOriginal, HookMetaInfo& hookMetaInfo)
{
    if (!pfnOriginal) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutexHook);
    auto                        it = g_vtHookRecord.find(pfnOriginal);
    if (it == g_vtHookRecord.end()) {
        return false;
    }

    hookMetaInfo = it->second;
    return true;
}