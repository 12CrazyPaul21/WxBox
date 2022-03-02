#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

#ifndef min
#define min WXBOX_MIN
#endif

#ifndef max
#define max WXBOX_MAX
#endif

#include <atlimage.h>

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

static bool Shell_Windows(const std::string& command, const std::vector<std::string>& args)
{
    if (command.empty()) {
        return false;
    }

    std::stringstream parameters;
    for (auto arg : args) {
        parameters << arg << " ";
    }

    return ::ShellExecuteA(NULL, "open", command.c_str(), parameters.str().c_str(), nullptr, SW_SHOWNORMAL) > (HINSTANCE)32;
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

static BOOL CALLBACK CollectMonitorRegionEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    WXBOX_UNREF(hdcMonitor);
    WXBOX_UNREF(lprcMonitor);

    auto multiMonitorRegionInfo = reinterpret_cast<wb_platform::PMultiMonitorRegionInfo>(dwData);
    if (!multiMonitorRegionInfo) {
        return FALSE;
    }

    // get monitor device name
    MONITORINFOEX monitorInfoEx;
    monitorInfoEx.cbSize = sizeof(monitorInfoEx);
    if (!GetMonitorInfoA(hMonitor, &monitorInfoEx)) {
        return TRUE;
    }

    // get monitor's physical size and position
    DEVMODEA devMode;
    ZeroMemory(&devMode, sizeof(devMode));
    devMode.dmSize = sizeof(devMode);
    if (!EnumDisplaySettingsA(monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &devMode)) {
        return TRUE;
    }

    wb_platform::MonitorPhysicalRegion monitorRegion(devMode.dmPosition.x, devMode.dmPosition.y, devMode.dmPelsWidth, devMode.dmPelsHeight);

    if (multiMonitorRegionInfo->monitorRegions.empty()) {
        multiMonitorRegionInfo->min_x = multiMonitorRegionInfo->max_x = monitorRegion.x;
        multiMonitorRegionInfo->min_y = multiMonitorRegionInfo->max_y = monitorRegion.y;
        multiMonitorRegionInfo->min_width = multiMonitorRegionInfo->max_width = monitorRegion.width;
        multiMonitorRegionInfo->min_height = multiMonitorRegionInfo->max_height = monitorRegion.height;
    }
    else {
        multiMonitorRegionInfo->min_x      = WXBOX_MIN(multiMonitorRegionInfo->min_x, monitorRegion.x);
        multiMonitorRegionInfo->max_x      = WXBOX_MAX(multiMonitorRegionInfo->max_x, monitorRegion.x);
        multiMonitorRegionInfo->min_y      = WXBOX_MIN(multiMonitorRegionInfo->min_y, monitorRegion.y);
        multiMonitorRegionInfo->max_y      = WXBOX_MAX(multiMonitorRegionInfo->max_y, monitorRegion.y);
        multiMonitorRegionInfo->min_width  = WXBOX_MIN(multiMonitorRegionInfo->min_width, monitorRegion.width);
        multiMonitorRegionInfo->max_width  = WXBOX_MAX(multiMonitorRegionInfo->max_width, monitorRegion.width);
        multiMonitorRegionInfo->min_height = WXBOX_MIN(multiMonitorRegionInfo->min_height, monitorRegion.height);
        multiMonitorRegionInfo->max_height = WXBOX_MAX(multiMonitorRegionInfo->max_height, monitorRegion.height);
    }

    multiMonitorRegionInfo->monitorRegions.emplace_back(std::move(monitorRegion));
    return TRUE;
}

static bool GetDesktopFullPhysicalRegion(wb_platform::FullMonitorPhysicalRegion& region)
{
    wb_platform::MultiMonitorRegionInfo multiMonitorRegionInfo;
    EnumDisplayMonitors(NULL, NULL, CollectMonitorRegionEnumProc, reinterpret_cast<LPARAM>(&multiMonitorRegionInfo));
    if (!multiMonitorRegionInfo.valid()) {
        return false;
    }

    region = multiMonitorRegionInfo.region();
    return true;
}

static bool DoCaptureMonitorSnap(const std::string& savePngImageFilePath, wb_platform::FullMonitorPhysicalRegion& region)
{
    //
    // config bitmap info
    //

    BITMAPINFOHEADER bmih;
    ZeroMemory(&bmih, sizeof(bmih));

    bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmih.biWidth         = region.width;
    bmih.biHeight        = region.height;
    bmih.biPlanes        = 1;
    bmih.biBitCount      = 24;
    bmih.biCompression   = BI_RGB;
    bmih.biSizeImage     = bmih.biWidth * bmih.biHeight * 3;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed       = 0;
    bmih.biClrImportant  = 0;

    //
    // create compatible desktop device-context and bitmap
    //

    HWND hWndDesktop = GetDesktopWindow();
    HDC  hDesktopDC  = GetDC(hWndDesktop);

    HDC hOffScreenDC = CreateCompatibleDC(hDesktopDC);
    if (!hOffScreenDC) {
        ReleaseDC(hWndDesktop, hDesktopDC);
        return false;
    }

    HBITMAP hOffScreenBmp = CreateDIBitmap(hDesktopDC, &bmih, 0, 0, nullptr, 0);
    if (!hOffScreenBmp) {
        DeleteDC(hOffScreenDC);
        ReleaseDC(hWndDesktop, hDesktopDC);
        return false;
    }

    //
    // capture desktop snap to offscreen bitmap
    //

    SelectObject(hOffScreenDC, hOffScreenBmp);
    BitBlt(hOffScreenDC, 0, 0, region.width, region.height, hDesktopDC, region.x, region.y, SRCCOPY);

    //
    // copy to image
    //

    CImage image;
    image.Attach(hOffScreenBmp);
    bool retval = SUCCEEDED(image.Save(savePngImageFilePath.c_str(), Gdiplus::ImageFormatPNG));

    DeleteObject(hOffScreenBmp);
    DeleteDC(hOffScreenDC);
    ReleaseDC(hWndDesktop, hDesktopDC);
    return retval;
}

static inline bool CaptureMainMonitorSnap_Windows(const std::string& savePngImageFilePath)
{
    RECT rect;
    if (!GetWindowRect(GetDesktopWindow(), &rect)) {
        return false;
    }

    wb_platform::FullMonitorPhysicalRegion region(rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top));
    return DoCaptureMonitorSnap(savePngImageFilePath, region);
}

static inline bool CaptureMonitorSnap_Windows(const std::string& savePngImageFilePath)
{
    wb_platform::FullMonitorPhysicalRegion region;
    if (!GetDesktopFullPhysicalRegion(region)) {
        return false;
    }

    return DoCaptureMonitorSnap(savePngImageFilePath, region);
}

#elif WXBOX_IN_MAC_OS

static bool Is64System_Mac()
{
    throw std::exception("Is64System_Mac stub");
    return false;
}

static bool Shell_Mac(const std::string& command, const std::vector<std::string>& args)
{
    throw std::exception("LockScreen stub");
    return false;
}

static std::string GetSystemVersionDescription_Mac()
{
    throw std::exception("GetSystemVersionDescription_Mac stub");
    return "";
}

static bool CaptureMainMonitorSnap_Mac(const std::string& savePngImageFilePath)
{
    throw std::exception("CaptureMainMonitorSnap_Mac stub");
    return false;
}

static bool CaptureMonitorSnap_Mac(const std::string& savePngImageFilePath)
{
    throw std::exception("CaptureMonitorSnap_Mac stub");
    return false;
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

void wxbox::util::platform::LockScreen()
{
#if WXBOX_IN_WINDOWS_OS
    ::LockWorkStation();
#elif WXBOX_IN_MAC_OS
    throw std::exception("LockScreen stub");
#endif
}

bool wxbox::util::platform::CaptureMainMonitorSnap(const std::string& savePngImageFilePath)
{
#if WXBOX_IN_WINDOWS_OS
    return CaptureMainMonitorSnap_Windows(savePngImageFilePath);
#elif WXBOX_IN_MAC_OS
    return CaptureMainMonitorSnap_Mac(savePngImageFilePath);
#endif
}

bool wxbox::util::platform::CaptureMonitorSnap(const std::string& savePngImageFilePath)
{
#if WXBOX_IN_WINDOWS_OS
    return CaptureMonitorSnap_Windows(savePngImageFilePath);
#elif WXBOX_IN_MAC_OS
    return CaptureMonitorSnap_Mac(savePngImageFilePath);
#endif
}

bool wxbox::util::platform::Shell(const std::string& command, const std::vector<std::string>& args)
{
#if WXBOX_IN_WINDOWS_OS
    return Shell_Windows(command, args);
#elif WXBOX_IN_MAC_OS
    return Shell_Mac(command, args);
#endif
}

//
// only for windows
//

#if WXBOX_IN_WINDOWS_OS

//
// Typedef
//

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemNotImplemented1,
    SystemProcessesAndThreadsInformation,
    SystemCallCounts,
    SystemConfigurationInformation,
    SystemProcessorTimes,
    SystemGlobalFlag,
    SystemNotImplemented2,
    SystemModuleInformation,
    SystemLockInformation,
    SystemNotImplemented3,
    SystemNotImplemented4,
    SystemNotImplemented5,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPagefileInformation,
    SystemInstructionEmulationCounts,
    SystemInvalidInfoClass1,
    SystemCacheInformation,
    SystemPoolTagInformation,
    SystemProcessorStatistics,
    SystemDpcInformation,
    SystemNotImplemented6,
    SystemLoadImage,
    SystemUnloadImage,
    SystemTimeAdjustment,
    SystemNotImplemented7,
    SystemNotImplemented8,
    SystemNotImplemented9,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemLoadAndCallImage,
    SystemPrioritySeparation,
    SystemNotImplemented10,
    SystemNotImplemented11,
    SystemInvalidInfoClass2,
    SystemInvalidInfoClass3,
    SystemTimeZoneInformation,
    SystemLookasideInformation,
    SystemSetTimeSlipEvent,
    SystemCreateSession,
    SystemDeleteSession,
    SystemInvalidInfoClass4,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemAddVerifier,
    SystemSessionProcessesInformation
} SYSTEM_INFORMATION_CLASS;

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectTypesInformation,
    ObjectHandleFlagInformation,
    ObjectSessionInformation,
    ObjectSessionObjectInformation,
    MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;

typedef LONG      NTSTATUS;
typedef NTSTATUS* PNTSTATUS;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR  ObjectTypeIndex;
    UCHAR  HandleAttributes;
    USHORT HandleValue;
    PVOID  Object;
    ULONG  GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG                          NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING  TypeName;
    ULONG           TotalNumberOfObjects;
    ULONG           TotalNumberOfHandles;
    ULONG           TotalPagedPoolUsage;
    ULONG           TotalNonPagedPoolUsage;
    ULONG           TotalNamePoolUsage;
    ULONG           TotalHandleTableUsage;
    ULONG           HighWaterNumberOfObjects;
    ULONG           HighWaterNumberOfHandles;
    ULONG           HighWaterPagedPoolUsage;
    ULONG           HighWaterNonPagedPoolUsage;
    ULONG           HighWaterNamePoolUsage;
    ULONG           HighWaterHandleTableUsage;
    ULONG           InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG           ValidAccessMask;
    BOOLEAN         SecurityRequired;
    BOOLEAN         MaintainHandleCount;
    UCHAR           TypeIndex;
    CHAR            ReservedByte;
    ULONG           PoolType;
    ULONG           DefaultPagedPoolCharge;
    ULONG           DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef NTSTATUS(WINAPI* FnNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
typedef NTSTATUS(NTAPI* FnNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// wxbox::util::platform
//

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

    return std::string(tmp.get());
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

ucpulong_t wxbox::util::platform::GetPEModuleImageSize(const std::string& path)
{
    ucpulong_t imageSize = 0;

    if (!wb_file::IsPathExists(path)) {
        return imageSize;
    }

    // open file
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    // create file mapping
    HANDLE hFileMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, GetFileSize(hFile, NULL), NULL);
    if (!hFileMapping) {
        CloseHandle(hFile);
        return imageSize;
    }

    // map memory
    LPVOID lpFileBaseAddress = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (!lpFileBaseAddress) {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return imageSize;
    }

    // fetch size of image
    PIMAGE_NT_HEADERS pImageNtHeaders = ImageNtHeader(lpFileBaseAddress);
    if (pImageNtHeaders) {
        imageSize = pImageNtHeaders->OptionalHeader.SizeOfImage;
    }

    UnmapViewOfFile(lpFileBaseAddress);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);

    return imageSize;
}

PSYSTEM_HANDLE_INFORMATION Inner_GetAllKernelObjectsInfo(FnNtQuerySystemInformation fnNtQuerySystemInformation)
{
    static const ULONG s_largeBufferSize   = 256 * 0x400 * 0x400;
    static ULONG       s_initialBufferSize = 0x400;

    if (!fnNtQuerySystemInformation) {
        return nullptr;
    }

    ULONG                      bufferSize = s_initialBufferSize;
    PSYSTEM_HANDLE_INFORMATION buffer     = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(LocalAlloc(LPTR, bufferSize));
    if (!buffer) {
        return nullptr;
    }

    NTSTATUS status;
    while ((status = fnNtQuerySystemInformation(SystemHandleInformation, reinterpret_cast<PVOID>(buffer), bufferSize, nullptr)) == STATUS_INFO_LENGTH_MISMATCH) {
        LocalFree(buffer);
        bufferSize *= 2;

        if (bufferSize > s_largeBufferSize) {
            return nullptr;
        }

        buffer = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(LocalAlloc(LPTR, bufferSize));
        if (!buffer) {
            return nullptr;
        }
    }

    if (!NT_SUCCESS(status)) {
        LocalFree(buffer);
        return nullptr;
    }

    if (bufferSize < s_largeBufferSize) {
        s_initialBufferSize = bufferSize;
    }

    return buffer;
}

bool wxbox::util::platform::RemoveAllMatchKernelObject(const std::string& programName, const std::wstring& kernelObjectNamePattern)
{
    static FnNtQuerySystemInformation fnNtQuerySystemInformation = nullptr;
    static FnNtQueryObject            fnNtQueryObject            = nullptr;

    if (programName.empty() || kernelObjectNamePattern.empty()) {
        return false;
    }

    //
    // get NtQuerySystemInformation and NtQueryObject Windows Native API Address
    //

    if (!fnNtQuerySystemInformation || !fnNtQueryObject) {
        HMODULE hNtdll = LoadLibraryA("Ntdll.dll");
        if (!hNtdll) {
            return false;
        }

        fnNtQuerySystemInformation = (FnNtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        fnNtQueryObject            = (FnNtQueryObject)GetProcAddress(hNtdll, "NtQueryObject");

        if (!fnNtQuerySystemInformation || !fnNtQueryObject) {
            return false;
        }
    }

    //
    // get all kernel objects info
    //

    PSYSTEM_HANDLE_INFORMATION allKernelObjects = Inner_GetAllKernelObjectsInfo(fnNtQuerySystemInformation);
    if (!allKernelObjects) {
        return false;
    }

    //
    // search all match kernel object
    //

    HANDLE                                           hCurrentProcess = GetCurrentProcess();
    std::unordered_map<wb_process::PID, std::string> pid2binName;

    for (ULONG i = 0; i < allKernelObjects->NumberOfHandles; i++) {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO object = &allKernelObjects->Handles[i];

        // match program
        auto binNameIt = pid2binName.find(object->UniqueProcessId);
        if (binNameIt == pid2binName.end()) {
            wb_process::ProcessInfo pi;
            if (!wb_process::GetProcessInfoByPID(object->UniqueProcessId, pi)) {
                continue;
            }
            pid2binName[object->UniqueProcessId] = pi.filename;

            if (_stricmp(pi.filename.c_str(), programName.c_str())) {
                continue;
            }
        }
        else if (_stricmp(binNameIt->second.c_str(), programName.c_str())) {
            continue;
        }

        //
        // match kernel object name pattern
        //

        bool matched = false;

        // open process handle
        auto hProcess = wb_process::OpenProcessAutoHandle(object->UniqueProcessId);
        if (!hProcess.hProcess) {
            continue;
        }

        // duplicate kernel object
        HANDLE hDuplicateHandle = NULL;
        if (!DuplicateHandle(hProcess.hProcess, (HANDLE)object->HandleValue, hCurrentProcess, &hDuplicateHandle, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS)) {
            continue;
        }

        // get kernel object name and match
        ULONG uNeededSize = 0;
        if (fnNtQueryObject(hDuplicateHandle, ObjectNameInformation, nullptr, 0, &uNeededSize) == STATUS_INFO_LENGTH_MISMATCH) {
            POBJECT_NAME_INFORMATION pObjectName = reinterpret_cast<POBJECT_NAME_INFORMATION>(LocalAlloc(LPTR, uNeededSize));
            if (!pObjectName) {
                CloseHandle(hDuplicateHandle);
                continue;
            }

            if (NT_SUCCESS(fnNtQueryObject(hDuplicateHandle, ObjectNameInformation, pObjectName, uNeededSize, &uNeededSize))) {
                if (pObjectName->Name.Buffer && wcsstr(pObjectName->Name.Buffer, kernelObjectNamePattern.c_str())) {
                    matched = true;
                }
            }

            LocalFree(pObjectName);
        }
        CloseHandle(hDuplicateHandle);

        if (!matched) {
            continue;
        }

        // delete kernel object
        if (DuplicateHandle(hProcess.hProcess, (HANDLE)object->HandleValue, hCurrentProcess, &hDuplicateHandle, DUPLICATE_CLOSE_SOURCE, FALSE, DUPLICATE_CLOSE_SOURCE)) {
            CloseHandle(hDuplicateHandle);
        }
    }

    LocalFree(allKernelObjects);
    return true;
}

#endif