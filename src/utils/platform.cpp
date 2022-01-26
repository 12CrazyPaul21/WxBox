#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

static bool Is64System_Windows()
{
    using FnGetNativeSystemInfo = void(WINAPI*)(LPSYSTEM_INFO lpSystemInfo);

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        return false;
    }

    FnGetNativeSystemInfo fnGetNativeSystemInfo = (FnGetNativeSystemInfo)GetProcAddress(hKernel32, "GetNativeSystemInfo");
    if (!fnGetNativeSystemInfo) {
        return false;
    }

    SYSTEM_INFO si = {0};
    fnGetNativeSystemInfo(&si);

    return (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64);
}

static std::string GetSystemVersionDescription_Windows()
{
    using FnRtlGetVersion = LONG(WINAPI*)(LPOSVERSIONINFOEXW);

    std::string productName = wb_platform::GetStringValueInRegister(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)", R"(ProductName)");
    if (productName.empty()) {
        productName = "Windows";
    }

    HMODULE hNtdll = GetModuleHandleA("ntdll");
    if (!hNtdll) {
        return productName;
    }

    FnRtlGetVersion fnRtlGetVersion = (FnRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
    if (!fnRtlGetVersion) {
        return productName;
    }

    OSVERSIONINFOEXW osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    if (fnRtlGetVersion(&osVersionInfo)) {
        return productName;
    }

    std::stringstream ss;
    ss << productName << " Build-" << std::dec << osVersionInfo.dwMajorVersion << "." << osVersionInfo.dwMinorVersion << "." << osVersionInfo.dwBuildNumber
       << " " << (wb_platform::Is64System() ? "64" : "32") << "-bits";
    return ss.str();
}

#elif WXBOX_IN_MAC_OS

static bool Is64System_Mac()
{
    throw std::exception("Is64System_Mac stub");
    return false;
}

static std::string GetSystemVersionDescription_Mac()
{
    throw std::exception("GetSystemVersionDescription_Mac stub");
    return "";
}

#endif

bool wxbox::util::platform::Is64System()
{
#if WXBOX_IN_WINDOWS_OS
    return Is64System_Windows();
#elif WXBOX_IN_MAC_OS
    return Is64System_Mac();
#endif
}

std::string wxbox::util::platform::GetCPUProductBrandDescription()
{
#if WXBOX_IN_WINDOWS_OS

    int  cpuInfo[4] = {-1};
    char cpuBrandString[0x40];

    memset(cpuBrandString, 0, sizeof(cpuBrandString));

    __cpuid(cpuInfo, 0x80000002);
    memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));

    __cpuid(cpuInfo, 0x80000003);
    memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));

    __cpuid(cpuInfo, 0x80000004);
    memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));

    return cpuBrandString;

#elif WXBOX_IN_MAC_OS

    throw std::exception("GetCPUProductBrandDescription stub");
    return "";

#endif
}

std::string wxbox::util::platform::GetSystemVersionDescription()
{
#if WXBOX_IN_WINDOWS_OS
    return GetSystemVersionDescription_Windows();
#elif WXBOX_IN_MAC_OS
    return GetSystemVersionDescription_Mac();
#endif
}

//
// only for windows
//

#if WXBOX_IN_WINDOWS_OS

bool wxbox::util::platform::EnableDebugPrivilege(bool bEnablePrivilege)
{
    HANDLE hToken = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    LUID seDebugNameLuid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &seDebugNameLuid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tokenPrivileges;
    tokenPrivileges.PrivilegeCount           = 1;
    tokenPrivileges.Privileges[0].Luid       = seDebugNameLuid;
    tokenPrivileges.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    bool retval = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);

    return retval && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

std::string wxbox::util::platform::GetStringValueInRegister(HKEY hKey, const char* subKey, const char* valueName)
{
    LSTATUS status = ERROR_SUCCESS;
    DWORD   cbData = 0;

    status = RegGetValueA(hKey, subKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    std::unique_ptr<char[]> tmp(new char[cbData]);
    status = RegGetValueA(hKey, subKey, valueName, RRF_RT_REG_SZ, nullptr, tmp.get(), &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    return std::move(std::string(tmp.get()));
}

const char* wxbox::util::platform::ExceptionDescription(const DWORD code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:
            return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "EXCEPTION_FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:
            return "EXCEPTION_FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "EXCEPTION_FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:
            return "EXCEPTION_FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:
            return "EXCEPTION_FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:
            return "EXCEPTION_FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:
            return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:
            return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:
            return "EXCEPTION_INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:
            return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:
            return "EXCEPTION_SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:
            return "EXCEPTION_STACK_OVERFLOW";
    }
    return "UNKNOWN EXCEPTION";
}

#endif