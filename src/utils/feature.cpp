#include <utils/common.h>

//
// Classes or Structures
//

/**
 * wxbox::util::feature::WxAbsoluteHookInfo
 */
uint32_t wb_feature::_WxAbsoluteHookInfo::GetApiRva(const std::string& api)
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

bool wb_feature::_WxApiHookInfo::GetWxHookPointFeaturesWithVersion(const std::string& version, WxHookPointFeatures& wxHookPointFeatures)
{
    if (mapWxHookPointFeatures.find(version) == mapWxHookPointFeatures.end()) {
        return false;
    }

    wxHookPointFeatures = mapWxHookPointFeatures[version];

    return true;
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

//
// Functions
//

static inline uint32_t UnwindEntryRVA(const YAML::Node& hookInfo, const std::string& funcName)
{
    uint32_t rva = 0;

    if (!funcName.length()) {
        return rva;
    }

    try {
        if (hookInfo[funcName].IsMap() && hookInfo[funcName]["EntryRVA"].IsScalar()) {
            rva = hookInfo[funcName]["EntryRVA"].as<uint32_t>();
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
        hookPointFeatureInfo.refFeatureStream = std::move(hookPointFeatureNode["RefFeatureStream"].as<std::vector<uint8_t>>());

        if (hookPointFeatureNode["RefBackExtralInstruction"].IsSequence()) {
            hookPointFeatureInfo.refBackExtralInstruction = std::move(hookPointFeatureNode["RefBackExtralInstruction"].as<std::vector<uint8_t>>());
        }

        if (hookPointFeatureNode["RefFrontExtralInstruction"].IsSequence()) {
            hookPointFeatureInfo.refFrontExtralInstruction = std::move(hookPointFeatureNode["RefFrontExtralInstruction"].as<std::vector<uint8_t>>());
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

        // PushInstruction
        hookPointFeatureInfo.pushInstruction = std::move(hookPointFeatureNode["PushInstruction"].as<std::vector<uint8_t>>());

        // RefFeatureStreams
        auto refFeatureStreamsNode = hookPointFeatureNode["RefFeatureStreams"];
        for (auto refFeatureStreamsElementNode : refFeatureStreamsNode) {
            if (!refFeatureStreamsElementNode.IsSequence()) {
                return false;
            }
            hookPointFeatureInfo.refFeatureStreams.emplace_back(refFeatureStreamsElementNode.as<std::vector<uint8_t>>());
        }
    }
    else if (!hookPointFeatureInfo.scanType.compare("instruction")) {
        // for 'instruction'

        if (!hookPointFeatureNode["InstructionFeatureStream"].IsSequence()) {
            return false;
        }

        hookPointFeatureInfo.instructionFeatureStream = std::move(hookPointFeatureNode["InstructionFeatureStream"].as<std::vector<uint8_t>>());
    }

    //
    // collect 'Locate' info
    //

    if (!hookPointFeatureNode["LocateActionFeatureStream"].IsSequence()) {
        return false;
    }
    if (!hookPointFeatureNode["HookPointOffset"].IsScalar()) {
        return false;
    }

    hookPointFeatureInfo.locateActionFeatureStream = std::move(hookPointFeatureNode["LocateActionFeatureStream"].as<std::vector<uint8_t>>());
    hookPointFeatureInfo.hookPointOffset           = std::move(hookPointFeatureNode["HookPointOffset"].as<int32_t>());

    if (!hookPointFeatureInfo.locateAction.compare("backThenFront")) {
        // for 'backThenFront'

        if (!hookPointFeatureNode["ThenLocateActionFeatureStream"].IsSequence()) {
            return false;
        }

        hookPointFeatureInfo.thenLocateActionFeatureStream = std::move(hookPointFeatureNode["ThenLocateActionFeatureStream"].as<std::vector<uint8_t>>());
    }
    else if (!hookPointFeatureInfo.locateAction.compare("backMultiTimes")) {
        // for 'backMultiTimes'

        if (!hookPointFeatureNode["LocateActionExecuteTimes"].IsScalar()) {
            return false;
        }

        hookPointFeatureInfo.locateActionExecuteTimes = hookPointFeatureNode["LocateActionExecuteTimes"].as<int32_t>();
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
        uint32_t rva = UnwindEntryRVA(hookInfo, api);
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