#include <utils/common.h>

#include <thirdparty/ldisasm.hpp>

wxbox::util::traits::FunctionInfo wxbox::util::traits::FetchFunctionInfo(void* pFunc, void* pFuncEnd, bool ignoreFilledInt3)
{
    FunctionInfo info;

    if (!pFunc || !pFuncEnd) {
        return info;
    }

    long funcBegin = (long)pFunc;
    long funcEnd   = (long)pFuncEnd;

    // 0xC3 == ret, 0xE9 is prefix of JMP far instruction
    if (((uint8_t*)funcEnd)[0] != 0xC3 && ((uint8_t*)funcEnd)[0] != 0xE9) {
        return info;
    }

    if (((uint8_t*)funcEnd)[0] == 0xE9) {
        // "/INCREMENTAL:YES" will use IAT
        funcEnd += *((long*)(funcEnd + 1)) + 1 + sizeof(long);
        funcBegin += *((long*)(funcBegin + 1)) + 1 + sizeof(long);
    }

    info.addr = (void*)funcBegin;
    info.size = std::abs(funcEnd - funcBegin);

    if (!info.size || !ignoreFilledInt3) {
        return info;
    }

    uint8_t* pCodeByte = (uint8_t*)info.addr;
    for (long i = info.size - 1; i >= 0 && pCodeByte[i] == 0xCC; i--, info.size--)
        ;

    return info;
}

void* wxbox::util::traits::GetActualEntryAddress(void* pfnEntry)
{
#if WXBOX_CPU_IS_X86
    if (((uint8_t*)pfnEntry)[0] != 0xE9) {
        return pfnEntry;
    }
    return (uint8_t*)pfnEntry + *((long*)((uint8_t*)pfnEntry + 1)) + 1 + sizeof(long);
#else
    return pfnEntry;
#endif
}

ucpulong_t wxbox::util::traits::CpuOpcodeInstructionByteSize(void* pBeginOpcode)
{
    if (!pBeginOpcode) {
        return 0;
    }

    return ldisasm(pBeginOpcode, WXBOX_CPU_IS_X86_64);
}
