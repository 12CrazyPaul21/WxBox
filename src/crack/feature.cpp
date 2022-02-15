#include <utils/common.h>

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

static inline std::vector<uint8_t> UnwindEntryFillStream(const YAML::Node& hookInfo, const std::string& funcName)
{
    std::vector<uint8_t> stream;

    if (!funcName.length()) {
        return stream;
    }

    try {
        if (hookInfo[funcName].IsMap() && hookInfo[funcName]["FillStream"].IsSequence()) {
            stream = hookInfo[funcName]["FillStream"].as<std::vector<uint8_t>>();
        }
    }
    catch (const std::exception& /*e*/) {
    }

    return stream;
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

        if (hookPointFeatureNode["RefFeatureStreamOffset"].IsScalar()) {
            hookPointFeatureInfo.refFeatureStreamOffset = hookPointFeatureNode["RefFeatureStreamOffset"].as<uint8_t>();
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

    //
    // collect Fill Stream
    //

    if (hookPointFeatureNode["FillStream"].IsSequence()) {
        hookPointFeatureInfo.fillStream = hookPointFeatureNode["FillStream"].as<std::vector<uint8_t>>();
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

        std::vector<uint8_t> fillStream = UnwindEntryFillStream(hookInfo, api);
        if (!fillStream.empty()) {
            absoluteHookInfo.mapApiFillStream.emplace(api, std::move(fillStream));
        }
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

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_Ref(const wb_feature::LocateTarget& locateTarget, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    // find the location of the refFeatureStream
    ucpulong_t va = wb_memory::ScanMemory(locateTarget.hProcess, locateTarget.pModuleBaseAddr, locateTarget.uModuleSize, hookPointFeatureInfo.refFeatureStream.data(), hookPointFeatureInfo.refFeatureStream.size());
    if (!va) {
        return 0;
    }

    // add offset
    va += hookPointFeatureInfo.refFeatureStreamOffset;

    std::vector<uint8_t> scanPattern;
    scanPattern.insert(scanPattern.end(), hookPointFeatureInfo.refBackExtralInstruction.begin(), hookPointFeatureInfo.refBackExtralInstruction.end());
    scanPattern.insert(scanPattern.end(), (uint8_t*)&va, (uint8_t*)&va + 4);
    scanPattern.insert(scanPattern.end(), hookPointFeatureInfo.refFrontExtralInstruction.begin(), hookPointFeatureInfo.refFrontExtralInstruction.end());

    // find the location of the reference address
    return wb_memory::ScanMemory(locateTarget.hProcess, locateTarget.pModuleBaseAddr, locateTarget.uModuleSize, scanPattern.data(), scanPattern.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_MultiPushRef(const wb_feature::LocateTarget& locateTarget, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    std::vector<uint8_t> scanPattern;

    for (size_t i = 0; i < hookPointFeatureInfo.refFeatureStreams.size(); i++) {
        // find the location of the featureStream
        auto       featureStream = hookPointFeatureInfo.refFeatureStreams[i];
        ucpulong_t va            = wb_memory::ScanMemory(locateTarget.hProcess, locateTarget.pModuleBaseAddr, locateTarget.uModuleSize, featureStream.data(), featureStream.size());
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
    return wb_memory::ScanMemory(locateTarget.hProcess, locateTarget.pModuleBaseAddr, locateTarget.uModuleSize, scanPattern.data(), scanPattern.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Scan_Type_Instruction(const wb_feature::LocateTarget& locateTarget, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemory(locateTarget.hProcess, locateTarget.pModuleBaseAddr, locateTarget.uModuleSize, hookPointFeatureInfo.instructionFeatureStream.data(), hookPointFeatureInfo.instructionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Scan(const wb_feature::LocateTarget& locateTarget, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (!hookPointFeatureInfo.scanType.compare("ref")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_Ref(locateTarget, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.scanType.compare("multiPushRef")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_MultiPushRef(locateTarget, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.scanType.compare("instruction")) {
        return LocateWxAPIHookPointVA_Step_Scan_Type_Instruction(locateTarget, hookPointFeatureInfo);
    }

    return 0;
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_Back(const wb_feature::LocateTarget& locateTarget, ucpulong_t pMemBegin, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemoryRev(locateTarget.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_Front(const wb_feature::LocateTarget& locateTarget, ucpulong_t pMemBegin, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    return wb_memory::ScanMemory(locateTarget.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_BackThenFront(const wb_feature::LocateTarget& locateTarget, ucpulong_t pMemBegin, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    ucpulong_t step1 = wb_memory::ScanMemoryRev(locateTarget.hProcess, (void*)pMemBegin, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
    if (!step1) {
        return 0;
    }

    return wb_memory::ScanMemory(locateTarget.hProcess, (void*)step1, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.thenLocateActionFeatureStream.data(), hookPointFeatureInfo.thenLocateActionFeatureStream.size());
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate_Action_BackMultiTimes(const wb_feature::LocateTarget& locateTarget, ucpulong_t pMemBegin, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    ucpulong_t addr = pMemBegin;

    for (long i = 0; i < hookPointFeatureInfo.locateActionExecuteTimes; i++) {
        addr = wb_memory::ScanMemoryRev(locateTarget.hProcess, (void*)addr, hookPointFeatureInfo.locateActionRange, hookPointFeatureInfo.locateActionFeatureStream.data(), hookPointFeatureInfo.locateActionFeatureStream.size());
        if (!addr) {
            return 0;
        }
    }

    return addr;
}

static inline ucpulong_t LocateWxAPIHookPointVA_Step_Locate(const wb_feature::LocateTarget& locateTarget, ucpulong_t pMemBegin, const wb_feature::HookPointFeatureInfo& hookPointFeatureInfo)
{
    if (!pMemBegin) {
        return 0;
    }

    ucpulong_t va = 0;

    if (!hookPointFeatureInfo.locateAction.compare("back")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_Back(locateTarget, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("front")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_Front(locateTarget, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backThenFront")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_BackThenFront(locateTarget, pMemBegin, hookPointFeatureInfo);
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backMultiTimes")) {
        va = LocateWxAPIHookPointVA_Step_Locate_Action_BackMultiTimes(locateTarget, pMemBegin, hookPointFeatureInfo);
    }

    if (va) {
        va += hookPointFeatureInfo.hookPointOffset;
    }

    return va;
}

//
// Classes or Structures
//

/**
 * wxbox::util::feature::WxAbsoluteHookInfo
 */
ucpulong_t wb_feature::_WxAbsoluteHookInfo::GetApiRva(const std::string& api) const
{
    if (mapApiRva.find(api) == mapApiRva.end()) {
        return 0;
    }

    return mapApiRva[api];
}

bool wb_feature::_WxAbsoluteHookInfo::GetApiFillStream(const std::string& api, std::vector<uint8_t>& stream) const
{
    if (mapApiFillStream.find(api) == mapApiFillStream.end()) {
        return false;
    }

    stream = mapApiFillStream[api];
    return true;
}

/**
 * wxbox::util::feature::WxHookPointFeatures
 */
bool wb_feature::_WxHookPointFeatures::GetApiHookFeature(const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo) const
{
    if (mapApiFeature.find(api) == mapApiFeature.end()) {
        return false;
    }

    hookPointFeatureInfo = mapApiFeature[api];
    return true;
}

bool wb_feature::_WxHookPointFeatures::GetApiHookFeatureFillStream(const std::string& api, std::vector<uint8_t>& stream) const
{
    if (mapApiFeature.find(api) == mapApiFeature.end()) {
        return false;
    }

    stream = mapApiFeature[api].fillStream;
    return true;
}

/**
 * wxbox::util::feature::WxApiFeatures
 */
bool wb_feature::_WxApiFeatures::IsThisWxVersionExplicitLocated(const std::string& version)
{
    wxbox::util::file::VersionNumber versionNumber;
    if (!wxbox::util::file::UnwindVersionNumber(version, versionNumber)) {
        return false;
    }

    return std::find(featureVersionList.begin(), featureVersionList.end(), versionNumber) != featureVersionList.end();
}

std::string wb_feature::_WxApiFeatures::FindSimilarVersion(const std::string& version)
{
    if (featureVersionList.empty()) {
        return "";
    }

    wxbox::util::file::VersionNumber versionNumber;
    if (!wxbox::util::file::UnwindVersionNumber(version, versionNumber)) {
        return "";
    }

    if (std::find(featureVersionList.begin(), featureVersionList.end(), versionNumber) != featureVersionList.end()) {
        return version;
    }

    wxbox::util::file::PVersionNumber similar = nullptr;
    for (size_t i = 0; i < featureVersionList.size(); i++) {
        if (featureVersionList[i] >= versionNumber) {
            similar = &featureVersionList[i];
            break;
        }
    }

    if (!similar) {
        similar = &featureVersionList[featureVersionList.size() - 1];
    }

    return similar->str;
}

//
// WxApiFeatures inner methods
//

bool wb_feature::_WxApiFeatures::Inner_LoadFeature(const std::string& version)
{
    wb_file::VersionNumber vn;
    if (!wb_file::UnwindVersionNumber(version, vn)) {
        return false;
    }

    std::string featureFileAbsPath = wb_file::JoinPath(featureFolderAbsPath, "v" + version + ".yml");
    if (!wb_file::IsPathExists(featureFileAbsPath)) {
        return false;
    }

    YAML::Node feature = wb_file::UnwindYamlFile(featureFileAbsPath);
    if (feature.IsNull()) {
        return false;
    }

    YAML::Node versionAbsoluteHookInfo = feature["absolute"];
    YAML::Node featureInfo             = feature["feature"];

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
        mapWxAbsoluteHookInfo[wxVersion] = std::move(absoluteHookInfo);
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
        mapWxHookPointFeatures[wxVersion] = std::move(wxHookPointFeatures);
    }

    return true;
}

wb_feature::WxAbsoluteHookInfo* wb_feature::_WxApiFeatures::Inner_GetAbsoluteHookInfo(const std::string& version)
{
    if (mapWxAbsoluteHookInfo.find(version) != mapWxAbsoluteHookInfo.end()) {
        return &mapWxAbsoluteHookInfo.at(version);
    }

    if (!Inner_LoadFeature(version) || mapWxAbsoluteHookInfo.find(version) == mapWxAbsoluteHookInfo.end()) {
        return nullptr;
    }

    return &mapWxAbsoluteHookInfo.at(version);
}

wb_feature::WxHookPointFeatures* wb_feature::_WxApiFeatures::Inner_GetHookPointFeatures(const std::string& version)
{
    if (mapWxHookPointFeatures.find(version) != mapWxHookPointFeatures.end()) {
        return &mapWxHookPointFeatures.at(version);
    }

    if (!Inner_LoadFeature(version) || mapWxHookPointFeatures.find(version) == mapWxHookPointFeatures.end()) {
        return nullptr;
    }

    return &mapWxHookPointFeatures.at(version);
}

wb_feature::WxHookPointFeatures* wb_feature::_WxApiFeatures::Inner_GetSimilarHookPointFeatures(const std::string& version)
{
    auto similarVersion = FindSimilarVersion(version);
    if (similarVersion.empty()) {
        return nullptr;
    }

    return Inner_GetHookPointFeatures(similarVersion);
}

//
// feature info
//

ucpulong_t wb_feature::_WxApiFeatures::GetAbsoluteHookPointRVA(const std::string& version, const std::string& api)
{
    auto ptr = Inner_GetAbsoluteHookInfo(version);
    if (!ptr) {
        return 0;
    }
    return ptr->GetApiRva(api);
}

bool wb_feature::_WxApiFeatures::GetAbsoluteHookInfo(const std::string& version, WxAbsoluteHookInfo& wxAbsoluteHookInfo)
{
    auto ptr = Inner_GetAbsoluteHookInfo(version);
    if (!ptr) {
        return false;
    }

    wxAbsoluteHookInfo = *ptr;
    return true;
}

bool wb_feature::_WxApiFeatures::GetHookPointFeature(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo)
{
    auto ptr = Inner_GetHookPointFeatures(version);
    if (!ptr) {
        return 0;
    }
    return ptr->GetApiHookFeature(api, hookPointFeatureInfo);
}

bool wb_feature::_WxApiFeatures::GetHookPointFeatures(const std::string& version, WxHookPointFeatures& wxHookPointFeatures)
{
    auto ptr = Inner_GetHookPointFeatures(version);
    if (!ptr) {
        return false;
    }

    wxHookPointFeatures = *ptr;
    return true;
}

bool wb_feature::_WxApiFeatures::GetSimilarHookPointFeature(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo)
{
    auto ptr = Inner_GetSimilarHookPointFeatures(version);
    if (!ptr) {
        return false;
    }

    return ptr->GetApiHookFeature(api, hookPointFeatureInfo);
}

bool wb_feature::_WxApiFeatures::GetSimilarHookPointFeatures(const std::string& version, WxHookPointFeatures& wxHookPointFeatures)
{
    auto ptr = Inner_GetSimilarHookPointFeatures(version);
    if (!ptr) {
        return false;
    }

    wxHookPointFeatures = *ptr;
    return true;
}

//
// hook point memory locate
//

ucpulong_t wb_feature::_WxApiFeatures::Locate(const LocateTarget& locateTarget, const std::string& version, const std::string& api)
{
    ucpulong_t va = AbsoluteLocate(locateTarget, version, api);
    if (va) {
        return va;
    }
    return FuzzyLocate(locateTarget, version, api);
}

ucpulong_t wb_feature::_WxApiFeatures::AbsoluteLocate(const LocateTarget& locateTarget, const WxAbsoluteHookInfo& wxAbsoluteHookInfo, const std::string& api)
{
    ucpulong_t rva = wxAbsoluteHookInfo.GetApiRva(api);
    return rva ? (ucpulong_t)locateTarget.pModuleBaseAddr + rva : 0;
}

ucpulong_t wb_feature::_WxApiFeatures::AbsoluteLocate(const LocateTarget& locateTarget, const std::string& version, const std::string& api)
{
    ucpulong_t rva = GetAbsoluteHookPointRVA(version, api);
    return rva ? (ucpulong_t)locateTarget.pModuleBaseAddr + rva : 0;
}

ucpulong_t wb_feature::_WxApiFeatures::FuzzyLocate(const LocateTarget& locateTarget, const HookPointFeatureInfo& hookPointFeatureInfo)
{
    //
    // step 'Scan'
    //

    ucpulong_t scanResultAddr = LocateWxAPIHookPointVA_Step_Scan(locateTarget, hookPointFeatureInfo);
    if (!scanResultAddr) {
        return 0;
    }

    //
    // step 'Locate'
    //

    return LocateWxAPIHookPointVA_Step_Locate(locateTarget, scanResultAddr, hookPointFeatureInfo);
}

ucpulong_t wb_feature::_WxApiFeatures::FuzzyLocate(const LocateTarget& locateTarget, const std::string& version, const std::string& api)
{
    wb_feature::HookPointFeatureInfo hookPointFeatureInfo;
    if (!GetSimilarHookPointFeature(version, api, hookPointFeatureInfo)) {
        return 0;
    }

    return FuzzyLocate(locateTarget, hookPointFeatureInfo);
}

//
// collect hook point
//

bool wb_feature::_WxApiFeatures::Collect(const LocateTarget& locateTarget, const WxAbsoluteHookInfo& wxAbsoluteHookInfo, WxAPIHookPointVACollection& vaCollection)
{
    for (auto api : wb_feature::WX_HOOK_API) {
        ucpulong_t rva = wxAbsoluteHookInfo.GetApiRva(api);
        if (!rva) {
            return false;
        }

        vaCollection.set(api, (ucpulong_t)locateTarget.pModuleBaseAddr + rva);
    }

    return true;
}

bool wb_feature::_WxApiFeatures::Collect(const LocateTarget& locateTarget, const WxHookPointFeatures& wxHookPointFeatures, WxAPIHookPointVACollection& vaCollection)
{
    for (auto api : wb_feature::WX_HOOK_API) {
        wb_feature::HookPointFeatureInfo hookPointFeatureInfo;
        if (!wxHookPointFeatures.GetApiHookFeature(api, hookPointFeatureInfo)) {
            return false;
        }

        ucpulong_t va = FuzzyLocate(locateTarget, hookPointFeatureInfo);
        if (!va) {
            return false;
        }

        vaCollection.set(api, va);
    }

    return true;
}

bool wb_feature::_WxApiFeatures::Collect(const wxbox::util::process::ProcessInfo& pi, const std::string& featureVersion, bool absoluteLocate, WxAPIHookPointVACollection& vaCollection)
{
    wxbox::util::process::ModuleInfo modInfo;
    if (!wxbox::util::process::GetModuleInfo(pi.pid, WX_WE_CHAT_CORE_MODULE, modInfo)) {
        return false;
    }

    wb_process::PROCESS_HANDLE hProcess;

#if WXBOX_IN_WINDOWS_OS
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.pid);
#elif WXBOX_IN_MAC_OS

#endif

    bool                                   bSuccess     = false;
    wb_feature::LocateTarget               locateTarget = {hProcess, modInfo.pModuleBaseAddr, modInfo.uModuleSize};
    wb_feature::WxAPIHookPointVACollection collection;

    if (absoluteLocate) {
        auto ptr = Inner_GetAbsoluteHookInfo(featureVersion);
        if (ptr) {
            bSuccess = Collect(locateTarget, *ptr, collection);
        }
    }
    else {
        auto ptr = Inner_GetHookPointFeatures(featureVersion);
        if (ptr) {
            bSuccess = Collect(locateTarget, *ptr, collection);
        }
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

bool wb_feature::_WxApiFeatures::Collect(const wxbox::util::process::ProcessInfo& pi, WxAPIHookPointVACollection& vaCollection)
{
    wb_wx::WeChatEnvironmentInfo wxEnvInfo;
    if (!wxbox::crack::wx::ResolveWxEnvInfo(pi.pid, wxEnvInfo)) {
        return false;
    }

    if (IsThisWxVersionExplicitLocated(wxEnvInfo.version)) {
        return Collect(pi, wxEnvInfo.version, true, vaCollection);
    }
    else {
        return Collect(pi, FindSimilarVersion(wxEnvInfo.version), false, vaCollection);
    }
}

//
// obtain fill stream
//

bool wb_feature::_WxApiFeatures::ObtainFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream)
{
    if (ObtainAbsoluteFillStream(version, api, stream)) {
        return true;
    }
    return ObtainFuzzyFillStream(version, api, stream);
}

bool wb_feature::_WxApiFeatures::ObtainAbsoluteFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream)
{
    auto ptr = Inner_GetAbsoluteHookInfo(version);
    if (!ptr) {
        return false;
    }

    return ptr->GetApiFillStream(api, stream);
}

bool wb_feature::_WxApiFeatures::ObtainFuzzyFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream)
{
    auto ptr = Inner_GetHookPointFeatures(version);
    if (!ptr) {
        return false;
    }

    return ptr->GetApiHookFeatureFillStream(api, stream);
}

//
// wxbox::util::feature
//

bool wxbox::crack::feature::ParseFeatureRepoList(const char* rawFeatureList, FeatureRepoList& repoFeatureList)
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

std::vector<std::string> wxbox::crack::feature::FeatureVersionList(const std::string& featuresPath)
{
    std::vector<std::string> result;

    if (!wb_file::IsPathExists(featuresPath)) {
        return result;
    }

    for (auto fileName : wb_file::ListFilesInDirectoryWithExt(featuresPath, "yml")) {
        auto featureVersion = wb_file::ExtractFileNameAndExt(fileName).first;
        if (featureVersion.size() < 2) {
            continue;
        }

        featureVersion = (featureVersion.data() + 1);
        if (wb_file::CheckVersionNumberValid(featureVersion)) {
            result.emplace_back(featureVersion);
        }
    }

    return result;
}

bool wxbox::crack::feature::PreLoadFeatures(const std::string& featuresPath, WxApiFeatures& wxApiFeatures)
{
    if (!wb_file::IsPathExists(featuresPath)) {
        return false;
    }

    wxApiFeatures.Reset();
    wxApiFeatures.platform             = WXBOX_PLATFORM_NAME;
    wxApiFeatures.featureFolderAbsPath = featuresPath;

    auto versionList = FeatureVersionList(featuresPath);
    for (auto v : versionList) {
        wb_file::VersionNumber vn;
        if (wb_file::UnwindVersionNumber(v, vn)) {
            wxApiFeatures.featureVersionList.emplace_back(std::move(vn));
        }
    }

    if (versionList.empty()) {
        return false;
    }

    std::sort(wxApiFeatures.featureVersionList.begin(), wxApiFeatures.featureVersionList.end());
    return true;
}