#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS

//
// Macro
//

#define WX_INSTALLATION_PATH_REGISTER_SUB_KEY "SOFTWARE\\Tencent\\WeChat"
#define WX_INSTALLATION_PATH_REGISTER_VALUE_NAME "InstallPath"

#define WX_WE_CHAT_EXE "WeChat.exe"
#define WX_WE_CHAT_WIN_DLL "WeChatWin.dll"

#define DEBUG_MODE_WAITING_INTERVAL_MS 10000

//
// Function
//

static inline std::string GetWxInstallationPath_Windows()
{
    LSTATUS status = ERROR_SUCCESS;
    DWORD   cbData = 0;

    // calc installation path string buffer bytes count
    status = RegGetValueA(HKEY_CURRENT_USER,
                          WX_INSTALLATION_PATH_REGISTER_SUB_KEY,
                          WX_INSTALLATION_PATH_REGISTER_VALUE_NAME,
                          RRF_RT_REG_SZ,
                          nullptr,
                          nullptr,
                          &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    std::unique_ptr<char[]> tmp(new char[cbData]);
    status = RegGetValueA(HKEY_CURRENT_USER,
                          WX_INSTALLATION_PATH_REGISTER_SUB_KEY,
                          WX_INSTALLATION_PATH_REGISTER_VALUE_NAME,
                          RRF_RT_REG_SZ,
                          nullptr,
                          tmp.get(),
                          &cbData);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    return std::move(std::string(tmp.get()));
}

static inline bool OpenWxWithMultiBoxing_CheckWeChatWinModule(HANDLE hProcess, const LOAD_DLL_DEBUG_INFO& loadDllDebugInfo, LPDWORD pdwModuleSize = nullptr)
{
    if (!hProcess || !loadDllDebugInfo.lpImageName) {
        return false;
    }

    BOOL    status                         = FALSE;
    SIZE_T  nNumberOfBytesRead             = 0;
    LPVOID  lpImageName                    = nullptr;
    uint8_t dllAbsPathBuffer[MAX_PATH * 2] = {0};

    // read image full path actual address
    status = ::ReadProcessMemory(hProcess, loadDllDebugInfo.lpImageName, &lpImageName, sizeof(lpImageName), &nNumberOfBytesRead);
    if (!status || !nNumberOfBytesRead || !lpImageName) {
        return false;
    }

    //  read image full path
    status = ::ReadProcessMemory(hProcess, lpImageName, (LPVOID)dllAbsPathBuffer, sizeof(dllAbsPathBuffer), &nNumberOfBytesRead);
    if (!status || !nNumberOfBytesRead) {
        return false;
    }

    // to string
    std::string moduleAbsPath;
    if (loadDllDebugInfo.fUnicode) {
        moduleAbsPath = wb_string::ToString((wchar_t*)dllAbsPathBuffer);
    }
    else {
        moduleAbsPath = (char*)dllAbsPathBuffer;
    }

    // get module file name
    std::string moduleFileName = wb_file::ToFileName(moduleAbsPath);

    // check for WeChatWin.dll
    if (::_stricmp(moduleFileName.c_str(), WX_WE_CHAT_WIN_DLL)) {
        return false;
    }

    // get pe info
    if (pdwModuleSize) {
        PE32Struct pe32Struct = {0};
        if (!GetPE32DataEx(moduleAbsPath.c_str(), &pe32Struct)) {
            return false;
        }
        *pdwModuleSize = pe32Struct.NtSizeOfImage;
    }

    return true;
}

static inline bool OpenWxWithMultiBoxing_Crack(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, HANDLE hProcess, LPVOID pBaseAddr, DWORD dwModuleSize)
{
    if (!hProcess) {
        return false;
    }

    wchar_t feature[] = L"_WeChat_App_Instance_Identity_Mutex_Name";

    // locate feature string address
    ULONG_PTR featAddress = wb_memory::ScanMemory(hProcess, pBaseAddr, dwModuleSize, (LPVOID)feature, sizeof(feature));
    if (!featAddress) {
        return false;
    }

    // locate feature ref address
    ULONG_PTR featRefAddress = wb_memory::ScanMemory(hProcess, pBaseAddr, dwModuleSize, (LPVOID)&featAddress, sizeof(featAddress));
    if (!featRefAddress) {
        return false;
    }

    // locate CheckAppSingleton function entry address
    uint8_t  funcEntryFeature_CheckAppSingleton[] = {0xCC, 0xCC, 0x55, 0x8B, 0xEC};
    uint32_t checkAppSingletonEntryAddr           = wb_memory::ScanMemoryRev(hProcess, (LPVOID)featRefAddress, 0x20, funcEntryFeature_CheckAppSingleton, sizeof(funcEntryFeature_CheckAppSingleton));
    if (!checkAppSingletonEntryAddr) {
        return false;
    }
    checkAppSingletonEntryAddr += 2;

    //
    // asm 'ret' code. [0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3](mov eax, 0; ret) can be used directly, but frida-gum's GumX86Writer is still used for assembly here
    //

    GumX86Writer x86Writer;
    guint8       x86MachineInstruction[32] = {0};
    guint        uInstructionSize          = 0;

    ::gum_x86_writer_init(&x86Writer, x86MachineInstruction);
    ::gum_x86_writer_set_target_cpu(&x86Writer, GUM_CPU_IA32);
    ::gum_x86_writer_set_target_abi(&x86Writer, GUM_ABI_WINDOWS);
    ::gum_x86_writer_put_mov_reg_u32(&x86Writer, GUM_REG_EAX, 0);
    ::gum_x86_writer_put_ret(&x86Writer);
    uInstructionSize = ::gum_x86_writer_offset(&x86Writer);

    //
    // crack wechat
    //

    SIZE_T uNumberOfBytesWritten = 0;
    return ::MemoryWriteSafe(hProcess, (LPVOID)checkAppSingletonEntryAddr, (LPVOID)x86MachineInstruction, (SIZE_T)uInstructionSize, &uNumberOfBytesWritten);
}

static inline bool OpenWxWithMultiBoxing_DebugLoop(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo, wb_process::PID pid)
{
    if (!pid) {
        return false;
    }

    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        return false;
    }

    bool        result       = false;
    DEBUG_EVENT debugEvent   = {0};
    DWORD       dwModuleSize = 0;

    for (;;) {
        if (!::WaitForDebugEventEx(&debugEvent, DEBUG_MODE_WAITING_INTERVAL_MS)) {
            break;
        }

        //
        // only handle "dll onload event" in target process
        //

        if (debugEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && debugEvent.dwProcessId == pid) {
            if (OpenWxWithMultiBoxing_CheckWeChatWinModule(hProcess, debugEvent.u.LoadDll, &dwModuleSize)) {
                result = OpenWxWithMultiBoxing_Crack(wxEnvInfo, hProcess, debugEvent.u.LoadDll.lpBaseOfDll, dwModuleSize);
                break;
            }
        }

        if (!::ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE)) {
            break;
        }
    }

    ::DebugActiveProcessStop(pid);
    ::CloseHandle(hProcess);
    return result;
}

static inline bool OpenWxWithMultiBoxing_Windows(const wb_wx::WeChatEnvironmentInfo& wxEnvInfo)
{
    // check execute file path exist and is valid
    if (!wb_file::IsPathExists(wxEnvInfo.executeAbsPath)) {
        return false;
    }

    // create process and attach it
    wb_process::PID pid = wb_process::StartProcessAndAttach(wxEnvInfo.executeAbsPath);
    if (!pid) {
        return false;
    }

    // debug loop
    if (!OpenWxWithMultiBoxing_DebugLoop(wxEnvInfo, pid)) {
        return false;
    }

    return true;
}

#elif WXBOX_IN_MAC_OS

static inline std::string GetWxInstallationPath_Mac()
{
    return "";
}

static inline bool OpenWxWithMultiBoxing_Mac(const WeChatEnvironmentInfo& wxEnvInfo)
{
    return false;
}

#endif

std::string wxbox::util::wx::GetWxInstallationPath()
{
    std::string wxInstalllationPath = "";

#if WXBOX_IN_WINDOWS_OS
    wxInstalllationPath = GetWxInstallationPath_Windows();
#elif WXBOX_IN_MAC_OS
    wxInstalllationPath = GetWxInstallationPath_Mac();
#endif

    return wxInstalllationPath;
}

bool wxbox::util::wx::IsWxInstallationPathValid(const std::string& installPath)
{
    if (installPath.length() == 0 || !wb_file::IsPathExists(installPath)) {
        return false;
    }

    std::vector<std::string> files = {
#if WXBOX_IN_WINDOWS_OS
        WX_WE_CHAT_EXE,
        WX_WE_CHAT_WIN_DLL
#elif WXBOX_IN_MAC_OS
    // stub
#endif
    };

    for (auto file : files) {
        if (!wb_file::IsPathExists(wb_file::JoinPath(installPath, file))) {
            return false;
        }
    }

    return true;
}

std::string wxbox::util::wx::GetWxVersion(const std::string& installPath)
{
#if WXBOX_IN_WINDOWS_OS
    // get WeChat version info from WeChatWin.dll
    std::string weChatWinPath = wb_file::JoinPath(installPath, WX_WE_CHAT_WIN_DLL);

    DWORD dwHandle = 0;
    DWORD dwSize   = ::GetFileVersionInfoSizeA(weChatWinPath.c_str(), &dwHandle);
    if (!dwSize) {
        return "";
    }

    std::unique_ptr<uint8_t[]> buf(new uint8_t[dwSize]);
    if (!::GetFileVersionInfoA(weChatWinPath.c_str(), dwHandle, dwSize, buf.get())) {
        return "";
    }

    VS_FIXEDFILEINFO* pvFileInfo    = nullptr;
    UINT              uFileInfoSize = 0;
    if (!VerQueryValueA(buf.get(), "\\", (LPVOID*)&pvFileInfo, &uFileInfoSize)) {
        return "";
    }

    char strFileVersion[MAX_PATH] = {0};
    ::sprintf_s(strFileVersion, MAX_PATH, "%hu.%hu.%hu.%hu", HIWORD(pvFileInfo->dwFileVersionMS), LOWORD(pvFileInfo->dwFileVersionMS), HIWORD(pvFileInfo->dwFileVersionLS), LOWORD(pvFileInfo->dwFileVersionLS));
    return strFileVersion;

#elif WXBOX_IN_MAC_OS

#endif
}

bool wxbox::util::wx::ResolveWxEnvInfo(const std::string& installPath, PWeChatEnvironmentInfo pWxEnvInfo)
{
    if (!pWxEnvInfo || !wb_wx::IsWxInstallationPathValid(installPath)) {
        return false;
    }

    pWxEnvInfo->installPath = installPath;

#if WXBOX_IN_WINDOWS_OS
    pWxEnvInfo->executeFileName = WX_WE_CHAT_EXE;
#elif WXBOX_IN_MAC_OS
    pWxEnvInfo->executeFileName = "Unknown";
#endif

    pWxEnvInfo->executeAbsPath = wb_file::JoinPath(installPath, pWxEnvInfo->executeFileName);
    pWxEnvInfo->version        = wb_wx::GetWxVersion(installPath);

    return true;
}

bool wxbox::util::wx::OpenWxWithMultiBoxing(const WeChatEnvironmentInfo& wxEnvInfo)
{
#if WXBOX_IN_WINDOWS_OS
    return OpenWxWithMultiBoxing_Windows(wxEnvInfo);
#elif WXBOX_IN_MAC_OS
    return OpenWxWithMultiBoxing_Mac(wxEnvInfo);
#endif
}