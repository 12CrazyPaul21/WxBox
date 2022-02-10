#include <utils/common.h>

//
// Classes or Structures
//

/**
 * wxbox::util::feature::WxAbsoluteHookInfo
 */
ucpulong_t wb_feature::_WxAbsoluteHookInfo::GetApiRva(const std::string& api)
{
    if (mapApiRva.find(api) == mapApiRva.end()) {
        return 0;
    }

    return mapApiRva[api];
}

/**
 * wxbox::util::feature::WxHookPointFeatures
 */
bool wb_feature::_WxHookPointFeatures::GetApiHookFeature(const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (mapApiFeature.find(api) == mapApiFeature.end()) {
        return false;
    }

    hookPointFeatureInfo = mapApiFeature[api];

    return true;
}

/**
 * wxbox::util::feature::WxApiHookInfo
 */
bool wb_feature::_WxApiHookInfo::GetWxAbsoluteHookInfoWithVersion(const std::string& version, WxAbsoluteHookInfo& wxAbsoluteHookInfo)
{
    if (mapWxAbsoluteHookInfo.find(version) == mapWxAbsoluteHookInfo.end()) {
        return false;
    }

    wxAbsoluteHookInfo = mapWxAbsoluteHookInfo[version];

    return true;
}

ucpulong_t wb_feature::_WxApiHookInfo::GetWxAPIAbsoluteHookPointAddressWithVersion(const std::string& version, const std::string& api)
{
    WxAbsoluteHookInfo wxAbsoluteHookInfo;

    if (!GetWxAbsoluteHookInfoWithVersion(version, wxAbsoluteHookInfo)) {
        return 0;
    }

    return wxAbsoluteHookInfo.GetApiRva(api);
}

bool wb_feature::_WxApiHookInfo::GetWxHookPointFeaturesWithVersion(const std::string& version, WxHookPointFeatures& wxHookPointFeatures)
{
    if (mapWxHookPointFeatures.find(version) == mapWxHookPointFeatures.end()) {
        return false;
    }

    wxHookPointFeatures = mapWxHookPointFeatures[version];

    return true;
}

bool wb_feature::_WxApiHookInfo::GetWxAPIHookPointFeatureWithVersion(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo)
{
    WxHookPointFeatures wxHookPointFeatures;

    if (!GetWxHookPointFeaturesWithVersion(version, wxHookPointFeatures)) {
        return false;
    }

    return wxHookPointFeatures.GetApiHookFeature(api, hookPointFeatureInfo);
}

bool wb_feature::_WxApiHookInfo::GetWxHookPointFeaturesWithSimilarVersion(const std::string& version, WxHookPointFeatures& wxHookPointFeatures)
{
    if (mapWxHookPointFeatures.empty()) {
        return false;
    }

    wxbox::util::file::VersionNumber versionNumber;
    if (!wxbox::util::file::UnwindVersionNumber(version, versionNumber)) {
        return false;
    }

    if (GetWxHookPointFeaturesWithVersion(version, wxHookPointFeatures)) {
        return true;
    }

    std::vector<wxbox::util::file::VersionNumber> vtVersions;
    for (auto pair : mapWxHookPointFeatures) {
        wxbox::util::file::VersionNumber vn;
        if (!wxbox::util::file::UnwindVersionNumber(pair.first, vn)) {
            continue;
        }
        vtVersions.emplace_back(vn);
    }

    if (vtVersions.size() == 0) {
        return false;
    }

    // sort version number
    std::sort(vtVersions.begin(), vtVersions.end());

    // find similar
    wxbox::util::file::PVersionNumber similar = nullptr;
    for (size_t i = 0; i < vtVersions.size(); i++) {
        if (vtVersions[i] >= versionNumber) {
            similar = &vtVersions[i];
            break;
        }
    }

    if (!similar) {
        similar = &vtVersions[vtVersions.size() - 1];
    }
    wxHookPointFeatures = mapWxHookPointFeatures[similar->str];
    return true;
}

bool wb_feature::_WxApiHookInfo::GetWxAPIHookPointFeatureWithSimilarVersion(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo)
{
    WxHookPointFeatures wxHookPointFeatures;

    if (!GetWxHookPointFeaturesWithSimilarVersion(version, wxHookPointFeatures)) {
        return false;
    }

    return wxHookPointFeatures.GetApiHookFeature(api, hookPointFeatureInfo);
}

//
// Functions
//

static inline ucpulong_t UnwindEntryRVA(const YAML::Node& hookInfo, const std::string& funcName)
{
    ucpulong_t rva = 0;

    if (!funcName.length()) {
        return rva;
    }

    try {
        if (hookInfo[funcName].IsMap() && hookInfo[funcName]["EntryRVA"].IsScalar()) {
            rva = hookInfo[funcName]["EntryRVA"].as<ucpulong_t>();
        }
    }
    catch (const std::exception& /*e*/) {
    }

    return rva;
}

static inline bool UnwindFeature(const YAML::Node& featInfo, const std::string& hookPointName, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (!hookPointName.length()) {
        return false;
    }

    hookPointFeatureInfo.Reset();

    // check whether feature exists
    if (!featInfo[hookPointName].IsMap()) {
        return false;
    }

    auto hookPointFeatureNode = featInfo[hookPointName];

    // check ScanType and LocateAction exist
    if (!hookPointFeatureNode["ScanType"].IsScalar() || !hookPointFeatureNode["LocateAction"].IsScalar()) {
        return false;
    }

    // get ScanType and LocateAction
    hookPointFeatureInfo.scanType     = hookPointFeatureNode["ScanType"].as<std::string>();
    hookPointFeatureInfo.locateAction = hookPointFeatureNode["LocateAction"].as<std::string>();

    // check whether ScanType and LocateAction valid
    if (hookPointFeatureInfo.scanType.compare("ref") && hookPointFeatureInfo.scanType.compare("multiPushRef") && hookPointFeatureInfo.scanType.compare("instruction")) {
        return false;
    }
    if (hookPointFeatureInfo.locateAction.compare("back") && hookPointFeatureInfo.locateAction.compare("front") && hookPointFeatureInfo.locateAction.compare("backThenFront") && hookPointFeatureInfo.locateAction.compare("backMultiTimes")) {
        return false;
    }

    //
    // collect 'Scan' info
    //

    if (!hookPointFeatureInfo.scanType.compare("ref")) {
        // for 'ref'

        if (!hookPointFeatureNode["RefFeatureStream"].IsSequence()) {
            return false;
        }
        hookPointFeatureInfo.refFeatureStream = hookPointFeatureNode["RefFeatureStream"].as<std::vector<uint8_t>>();

        if (hookPointFeatureNode["RefBackExtralInstruction"].IsSequence()) {
            hookPointFeatureInfo.refBackExtralInstruction = hookPointFeatureNode["RefBackExtralInstruction"].as<std::vector<uint8_t>>();
        }

        if (hookPointFeatureNode["RefFrontExtralInstruction"].IsSequence()) {
            hookPointFeatureInfo.refFrontExtralInstruction = hookPointFeatureNode["RefFrontExtralInstruction"].as<std::vector<uint8_t>>();
        }
    }
    else if (!hookPointFeatureInfo.scanType.compare("multiPushRef")) {
        // for 'multiPushRef'

        if (!hookPointFeatureNode["PushInstruction"].IsSequence()) {
            return false;
        }
        if (!hookPointFeatureNode["RefFeatureStreams"].IsSequence()) {
            return false;
        }
        if (!hookPointFeatureNode["RefFeatureStreamsOffset"].IsSequence()) {
            return false;
        }

        // PushInstruction
        hookPointFeatureInfo.pushInstruction = hookPointFeatureNode["PushInstruction"].as<std::vector<uint8_t>>();

        // RefFeatureStreams
        auto refFeatureStreamsNode = hookPointFeatureNode["RefFeatureStreams"];
        for (auto refFeatureStreamsElementNode : refFeatureStreamsNode) {
            if (!refFeatureStreamsElementNode.IsSequence()) {
                return false;
            }
            hookPointFeatureInfo.refFeatureStreams.emplace_back(refFeatureStreamsElementNode.as<std::vector<uint8_t>>());
        }

        // RefFeatureStreamsOffset
        hookPointFeatureInfo.refFeatureStreamsOffset = hookPointFeatureNode["RefFeatureStreamsOffset"].as<std::vector<uint8_t>>();
    }
    else if (!hookPointFeatureInfo.scanType.compare("instruction")) {
        // for 'instruction'

        if (!hookPointFeatureNode["InstructionFeatureStream"].IsSequence()) {
            return false;
        }

        hookPointFeatureInfo.instructionFeatureStream = hookPointFeatureNode["InstructionFeatureStream"].as<std::vector<uint8_t>>();
    }

    //
    // collect 'Locate' info
    //

    if (!hookPointFeatureNode["LocateActionFeatureStream"].IsSequence()) {
        return false;
    }
    if (!hookPointFeatureNode["LocateActionRange"].IsScalar()) {
        return false;
    }
    if (!hookPointFeatureNode["HookPointOffset"].IsScalar()) {
        return false;
    }

    hookPointFeatureInfo.locateActionFeatureStream = hookPointFeatureNode["LocateActionFeatureStream"].as<std::vector<uint8_t>>();
    hookPointFeatureInfo.locateActionRange         = hookPointFeatureNode["LocateActionRange"].as<long>();
    hookPointFeatureInfo.hookPointOffset           = hookPointFeatureNode["HookPointOffset"].as<long>();

    if (!hookPointFeatureInfo.locateAction.compare("backThenFront")) {
        // for 'backThenFront'

        if (!hookPointFeatureNode["ThenLocateActionFeatureStream"].IsSequence()) {
            return false;
        }

        hookPointFeatureInfo.thenLocateActionFeatureStream = hookPointFeatureNode["ThenLocateActionFeatureStream"].as<std::vector<uint8_t>>();
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backMultiTimes")) {
        // for 'backMultiTimes'

        if (!hookPointFeatureNode["LocateActionExecuteTimes"].IsScalar()) {
            return false;
        }

        hookPointFeatureInfo.locateActionExecuteTimes = hookPointFeatureNode["LocateActionExecuteTimes"].as<long>();
    }

    return true;
}

static inline bool UnwindFeatureSecureWrapper(const YAML::Node& featInfo, const std::string& hookPointName, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    bool result = false;

    try {
        result = UnwindFeature(featInfo, hookPointName, hookPointFeatureInfo);
    }
    catch (const std::exception& /*e*/) {
    }

    return result;
}

static inline bool UnwindAbsoluteHookInfo(const std::string& wxVersion, const YAML::Node& hookInfo, wb_feature::WxAbsoluteHookInfo& absoluteHookInfo)
{
    for (auto api : wb_feature::WX_HOOK_API) {
        ucpulong_t rva = UnwindEntryRVA(hookInfo, api);
        if (!rva) {
            return false;
        }
        absoluteHookInfo.mapApiRva[api] = rva;
    }

    absoluteHookInfo.wxVersion = wxVersion;
    return true;
}

static inline bool UnwindFeatureInfo(const std::string& wxVersion, const YAML::Node& featInfo, wb_feature::WxHookPointFeatures& wxHookPointFeatures)
{
    wb_feature::HookPointFeatureInfo tmpHookPointFeatureInfo;

    wxHookPointFeatures.Reset();
    wxHookPointFeatures.wxVersion = wxVersion;

    for (auto api : wb_feature::WX_HOOK_API) {
        if (!UnwindFeatureSecureWrapper(featInfo, api, tmpHookPointFeatureInfo)) {
            return false;
        }

        wxHookPointFeatures.mapApiFeature[api] = tmpHookPointFeatureInfo;
    }

    return true;
}

static ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_Ref(wb_feature::LocateTargetInfo& locateTargetInfo, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    // find the location of the refFeatureStream
    ucpulong_t va = wb_memory::ScanMemory(locateTargetInfo.hProcess, locateTargetInfo.pModuleBaseAddr, locateTargetInfo.uModuleSize, hookPointFeatureInfo.refFeatureStream.data(), hookPointFeatureInfo.refFeatureStream.size());
    if (!va) {
        return 0;
    }

    std::vector<uint8_t> scanPattern;
    scanPattern.insert(scanPattern.end(), hookPointFeatureInfo.refBackExtralInstruction.begin(), hookPointFeatureInfo.refBackExtralInstruction.end());
    scanPattern.insert(scanPattern.end(), (uint8_t*)&va, (uint8_t*)&va + 4);
    scanPattern.insert(scanPattern.end(), hookPointFeatureInfo.refFrontExtralInstruction.begin(), hookPointFeatureInfo.refFrontExtralInstruction.end());

    // find the location of the reference address
    return wb_memory::ScanMemory(locateTargetInfo.hProcess, locateTargetInfo.pModuleBaseAddr, locateTargetInfo.uModuleSize, scanPattern.data(), scanPattern.size());
}

static ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_MultiPushRef(wb_feature::LocateTargetInfo& locateTargetInfo, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    std::vector<uint8_t> scanPattern;

    for (size_t i = 0; i < hookPointFeatureInfo.refFeatureStreams.size(); i++) {
        // find the location of the featureStream
        auto       featureStream = hookPointFeatureInfo.refFeatureStreams[i];
        ucpulong_t va            = wb_memory::ScanMemory(locateTargetInfo.hProcess, locateTargetInfo.pModuleBaseAddr, locateTargetInfo.uModuleSize, featureStream.data(), featureStream.size());
        if (!va) {
            return 0;
        }

        if (i < hookPointFeatureInfo.refFeatureStreamsOffset.size()) {
            va += hookPointFeatureInfo.refFeatureStreamsOffset[i];
        }

        scanPattern.insert(scanPattern.end(), hookPointFeatureInfo.pushInstruction.begin(), hookPointFeatureInfo.pushInstruction.end());
        scanPattern.insert(scanPattern.end(), (uint8_t*)&va, (uint8_t*)&va + 4);
    }

    // find the location of the reference address
    return wb_memory::ScanMemory(locateTargetInfo.hProcess, locateTargetInfo.pModuleBaseAddr, locateTargetInfo.uModuleSize, scanPattern.data(), scanPattern.size());
}

static ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_Instruction(wb_feature::LocateTargetInfo& locateTargetInfo, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemory(locateTargetInfo.hProcess, locateTargetInfo.pModuleBaseAddr, locateTargetInfo.uModuleSize, hookPointFeatureInfo.instructionFeatureStream.data(), hookPointFeatureInfo.instructionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Scan(wb_feature::LocateTargetInfo& locateTargetInfo, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (!hookPointFeatureInfo.scanType.compare("ref")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_Ref(locateTargetInfo, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.scanType.compare("multiPushRef")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_MultiPushRef(locateTargetInfo, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.scanType.compare("instruction")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_Instruction(locateTargetInfo, hookPointFeatureInfo);
    }

    return 0;
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_Back(wb_feature::LocateTargetInfo& locateTargetInfo, ucpulong_t pMemBegin, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemoryRev(locateTargetInfo.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_Front(wb_feature::LocateTargetInfo& locateTargetInfo, ucpulong_t pMemBegin, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemory(locateTargetInfo.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_BackThenFront(wb_feature::LocateTargetInfo& locateTargetInfo, ucpulong_t pMemBegin, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    ucpulong_t step1 = wb_memory::ScanMemoryRev(locateTargetInfo.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
    if (!step1) {
        return 0;
    }

    return wb_memory::ScanMemory(locateTargetInfo.hProcess, (void*)step1, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.thenLocateActionFeatureStream.data(), hookPointFeatureInfo.thenLocateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_BackMultiTimes(wb_feature::LocateTargetInfo& locateTargetInfo, ucpulong_t pMemBegin, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    ucpulong_t addr = pMemBegin;

    for (long i = 0; i < hookPointFeatureInfo.locateActionExecuteTimes; i++) {
        addr = wb_memory::ScanMemoryRev(locateTargetInfo.hProcess, (void*)addr, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
        if (!addr) {
            return 0;
        }
    }

    return addr;
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate(wb_feature::LocateTargetInfo& locateTargetInfo, ucpulong_t pMemBegin, wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (!pMemBegin) {
        return 0;
    }

    ucpulong_t va = 0;

    if (!hookPointFeatureInfo.locateAction.compare("back")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_Back(locateTargetInfo, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("front")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_Front(locateTargetInfo, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backThenFront")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_BackThenFront(locateTargetInfo, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backMultiTimes")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_BackMultiTimes(locateTargetInfo, pMemBegin, hookPointFeatureInfo);
    }

    if (va) {
        va += hookPointFeatureInfo.hookPointOffset;
    }

    return va;
}

bool wxbox::util::feature::UnwindFeatureConf(const std::string& confPath, wb_feature::WxApiHookInfo& wxApiHookInfo)
{
    if (!wb_file::IsPathExists(confPath)) {
        return false;
    }

    wxApiHookInfo.Reset();
    wxApiHookInfo.platform = WXBOX_PLATFORM_NAME;

    // load and parse yaml file
    YAML::Node root = wb_file::UnwindYamlFile(confPath);

    // check yaml file valid
    if (root.IsNull()) {
        return false;
    }

    // check feature info's version
    std::string version;
    if (root["version"].IsScalar()) {
        version = root["version"].as<std::string>();
    }

    // get config root path
    std::string confRootPath = wb_file::ToDirectoryPath(confPath);

    // check whether the "platform" is valid
    if (!root["platform"].IsMap()) {
        return false;
    }

    //
    // analyze platform related features
    //

    if (!root["platform"][wxApiHookInfo.platform].IsScalar()) {
        return false;
    }

    auto platformFeatureFileName     = root["platform"][wxApiHookInfo.platform].as<std::string>();
    wxApiHookInfo.featureFileAbsPath = wb_file::JoinPath(confRootPath, platformFeatureFileName);
    if (!wb_file::IsPathExists(wxApiHookInfo.featureFileAbsPath)) {
        return false;
    }

    YAML::Node platformFeature = wb_file::UnwindYamlFile(wxApiHookInfo.featureFileAbsPath);
    if (platformFeature.IsNull()) {
        return false;
    }

    YAML::Node versionAbsoluteHookInfo = platformFeature["absolute"];
    YAML::Node featureInfo             = platformFeature["feature"];

    // parse all absolute version info
    for (auto aVersion : versionAbsoluteHookInfo) {
        std::string wxVersion = aVersion.first.as<std::string>();
        YAML::Node  hookInfo  = aVersion.second;

        if (hookInfo.IsNull()) {
            continue;
        }

        // parse this version's hook info
        wb_feature::WxAbsoluteHookInfo absoluteHookInfo;
        if (!UnwindAbsoluteHookInfo(wxVersion, hookInfo, absoluteHookInfo)) {
            continue;
        }

        // record this version's absolute hook info
        wxApiHookInfo.mapWxAbsoluteHookInfo[wxVersion] = std::move(absoluteHookInfo);
    }

    // parse all feature
    for (auto aVersionFeature : featureInfo) {
        std::string wxVersion = aVersionFeature.first.as<std::string>();
        YAML::Node  featInfo  = aVersionFeature.second;

        if (featInfo.IsNull()) {
            continue;
        }

        // parse this version's hook feature info
        wb_feature::WxHookPointFeatures wxHookPointFeatures;
        if (!UnwindFeatureInfo(wxVersion, featInfo, wxHookPointFeatures)) {
            continue;
        }

        // record this version's hook feature info
        wxApiHookInfo.mapWxHookPointFeatures[wxVersion] = std::move(wxHookPointFeatures);
    }

    return true;
}

ucpulong_t wxbox::util::feature::LocateWxAPIHookPointVA(const wxbox::util::wx::WeChatEnvironmentInfo& wxEnvInfo, WxApiHookInfo& wxApiHookInfo, LocateTargetInfo locateTargetInfo, const std::string& api)
{
    ucpulong_t va = LocateWxAPIHookPointVAOnlyAbsolute(wxEnvInfo, wxApiHookInfo, locateTargetInfo, api);
    if (va) {
        return va;
    }
    return LocateWxAPIHookPointVAOnlyFeature(wxEnvInfo, wxApiHookInfo, locateTargetInfo, api);
}

ucpulong_t wxbox::util::feature::LocateWxAPIHookPointVAOnlyAbsolute(const wxbox::util::wx::WeChatEnvironmentInfo& wxEnvInfo, WxApiHookInfo& wxApiHookInfo, LocateTargetInfo locateTargetInfo, const std::string& api)
{
    ucpulong_t rva = wxApiHookInfo.GetWxAPIAbsoluteHookPointAddressWithVersion(wxEnvInfo.version, api);
    if (rva) {
        return (ucpulong_t)locateTargetInfo.pModuleBaseAddr + rva;
    }
    return 0;
}

ucpulong_t wxbox::util::feature::LocateWxAPIHookPointVAOnlyFeature(const wxbox::util::wx::WeChatEnvironmentInfo& wxEnvInfo, WxApiHookInfo& wxApiHookInfo, LocateTargetInfo locateTargetInfo, const std::string& api)
{
    wb_feature::HookPointFeatureInfo hookPointFeatureInfo;
    if (!wxApiHookInfo.GetWxAPIHookPointFeatureWithSimilarVersion(wxEnvInfo.version, api, hookPointFeatureInfo)) {
        return 0;
    }

    //
    // step 'Scan'
    //

    ucpulong_t scanResultAddr = LocateWxAPIHookPointVA_Step_Scan(locateTargetInfo, hookPointFeatureInfo);
    if (!scanResultAddr) {
        return 0;
    }

    //
    // step 'Locate'
    //

    return LocateWxAPIHookPointVA_Step_Locate(locateTargetInfo, scanResultAddr, hookPointFeatureInfo);
}

bool wxbox::util::feature::CollectWeChatProcessHookPointVA(const wxbox::util::process::ProcessInfo& pi, const WxApiHookInfo& wxApiHookInfo, WxAPIHookPointVACollection& vaCollection)
{
    //
    // note: !!! haven't checked whether the wxbot module exists !!!
    //

    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::util::wx::ResolveWxEnvInfo(pi.dirpath, &wxEnvInfo)) {
        return false;
    }

    wxbox::util::process::ModuleInfo modInfo;
    if (!wxbox::util::process::GetModuleInfo(pi.pid, WX_WE_CHAT_CORE_MODULE, modInfo)) {
        return false;
    }

    wb_process::PROCESS_HANDLE hProcess;

#if WXBOX_IN_WINDOWS_OS
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.pid);
#elif WXBOX_IN_MAC_OS

#endif

    bool                                   bSuccess         = true;
    wb_feature::LocateTargetInfo           locateTargetInfo = {hProcess, modInfo.pModuleBaseAddr, modInfo.uModuleSize};
    wb_feature::WxAPIHookPointVACollection collection;
    for (auto api : wb_feature::WX_HOOK_API) {
        ucpulong_t va = wb_feature::LocateWxAPIHookPointVA(wxEnvInfo, const_cast<WxApiHookInfo&>(wxApiHookInfo), locateTargetInfo, api);
        if (!va) {
            bSuccess = false;
            break;
        }
        collection.set(api, va);
    }

    if (bSuccess) {
        vaCollection = std::move(collection);
    }

#if WXBOX_IN_WINDOWS_OS
    CloseHandle(hProcess);
#elif WXBOX_IN_MAC_OS

#endif

    return bSuccess;
}

bool wxbox::util::feature::ParseRepoFeatureList(const char* rawFeatureList, RepoFeatureList& repoFeatureList)
{
    if (!rawFeatureList || !strlen(rawFeatureList)) {
        return false;
    }

    std::stringstream ss(rawFeatureList);
    std::string       line;

    //
    // fetch timestamp
    //

    if (!std::getline(ss, line, '\n')) {
        return false;
    }

    wb_string::Trim(line);
    auto timestampPair = wb_string::SplitString(line, " ");
    if (timestampPair.size() != 2 || timestampPair[0].compare("timestamp")) {
        return false;
    }
    repoFeatureList.timestamp = timestampPair[1];

    //
    // fetch all features
    //

    while (std::getline(ss, line, '\n')) {
        wb_string::Trim(line);
        auto featurePair = wb_string::SplitString(line, " ");
        if (featurePair.size() != 2) {
            continue;
        }

        repoFeatureList.features.emplace_back(std::make_pair(featurePair[0] + ".yml", featurePair[1]));
    }

    return true;
}