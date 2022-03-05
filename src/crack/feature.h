#ifndef __WXBOX_CRACK_FEATURE_H
#define __WXBOX_CRACK_FEATURE_H

namespace wxbox {
    namespace crack {
        namespace feature {

            //
            // Global
            //

            const std::vector<std::string> WX_HOOK_API = {
                "CheckAppSingleton",
                "FetchGlobalContactContextAddress",
                "InitWeChatContactItem",
                "DeinitWeChatContactItem",
                "FindAndDeepCopyWeChatContactItemWithWXIDWrapper",
                "FetchGlobalProfileContext",
                "HandleRawMessages",
                "HandleReceivedMessages",
                "WXSendTextMessage",
                "FetchGlobalSendMessageContext",
                "WXSendFileMessage",
                "CloseLoginWnd",
                "LogoutAndExitWeChat",
                "Logouted",
                "LogoutedByMobile",
                "Logined",
                "WeChatEventProc",
            };

            //
            // Typedef
            //

            typedef struct _FeatureRepoList
            {
                std::string                                      timestamp;
                std::vector<std::pair<std::string, std::string>> features;

                //
                // constructor
                //

                _FeatureRepoList() = default;

                SETUP_COPY_METHOD(_FeatureRepoList, other)
                {
                    timestamp = other.timestamp;
                    features  = other.features;
                }

                SETUP_MOVE_METHOD(_FeatureRepoList, other)
                {
                    timestamp = std::move(other.timestamp);
                    features  = std::move(other.features);
                }
            } FeatureRepoList, *PFeatureRepoList;

            typedef struct _WxAbsoluteHookInfo
            {
                std::string                                                   wxVersion;
                mutable std::unordered_map<std::string, ucpulong_t>           mapApiRva;
                mutable std::unordered_map<std::string, std::vector<uint8_t>> mapApiFillStream;

                //
                // constructor
                //

                _WxAbsoluteHookInfo() = default;

                SETUP_COPY_METHOD(_WxAbsoluteHookInfo, other)
                {
                    wxVersion        = other.wxVersion;
                    mapApiRva        = other.mapApiRva;
                    mapApiFillStream = other.mapApiFillStream;
                }

                SETUP_MOVE_METHOD(_WxAbsoluteHookInfo, other)
                {
                    wxVersion        = std::move(other.wxVersion);
                    mapApiRva        = std::move(other.mapApiRva);
                    mapApiFillStream = std::move(other.mapApiFillStream);
                }

                //
                // Method
                //

                void Reset()
                {
                    wxVersion = "";
                    mapApiRva.clear();
                    mapApiFillStream.clear();
                }

                ucpulong_t GetApiRva(const std::string& api) const;
                bool       GetApiFillStream(const std::string& api, std::vector<uint8_t>& stream) const;

            } WxAbsoluteHookInfo, *PWxAbsoluteHookInfo;

            typedef struct _HookPointFeatureInfo
            {
                //
                // ScanType:
                //  - ref: like char* and wchar_t*
                //  - multiPushRef: like multi times push (char* or wchar_t*)
                //  - instruction: instruction's va
                //

                std::string scanType;

                // for 'ref' scan type
                std::vector<uint8_t> refFeatureStream;
                std::vector<uint8_t> refBackExtralInstruction;
                std::vector<uint8_t> refFrontExtralInstruction;
                uint8_t              refFeatureStreamOffset;

                // for 'multiPushRef' scan type
                std::vector<uint8_t>              pushInstruction;
                std::vector<std::vector<uint8_t>> refFeatureStreams;
                std::vector<uint8_t>              refFeatureStreamsOffset;

                // for 'instruction' scan type
                std::vector<uint8_t> instructionFeatureStream;

                //
                // LocateAction:
                //  - back: high to low
                //  - front: low to high
                //  - backThenFront: high to low then to high
                //  - backMultiTimes: high to low, multi times
                //

                std::string          locateAction;
                std::vector<uint8_t> locateActionFeatureStream;
                long                 hookPointOffset;
                long                 locateActionRange;

                // for 'backThenFront' locate action
                std::vector<uint8_t> thenLocateActionFeatureStream;

                // for 'backMultiTimes' locate action
                long locateActionExecuteTimes;

                // fill stream
                std::vector<uint8_t> fillStream;

                //
                // constructor
                //

                _HookPointFeatureInfo()
                  : refFeatureStreamOffset(0)
                  , hookPointOffset(0)
                  , locateActionRange(0)
                  , locateActionExecuteTimes(0)
                {
                }

                SETUP_COPY_METHOD(_HookPointFeatureInfo, other)
                {
                    //
                    // scan info
                    //

                    scanType = other.scanType;

                    // for 'ref'
                    refFeatureStream          = other.refFeatureStream;
                    refBackExtralInstruction  = other.refBackExtralInstruction;
                    refFrontExtralInstruction = other.refFrontExtralInstruction;
                    refFeatureStreamOffset    = other.refFeatureStreamOffset;

                    // for 'multiPushRef'
                    pushInstruction         = other.pushInstruction;
                    refFeatureStreams       = other.refFeatureStreams;
                    refFeatureStreamsOffset = other.refFeatureStreamsOffset;

                    // for 'instruction'
                    instructionFeatureStream = other.instructionFeatureStream;

                    //
                    // locate action
                    //

                    locateAction              = other.locateAction;
                    locateActionFeatureStream = other.locateActionFeatureStream;
                    locateActionRange         = other.locateActionRange;
                    hookPointOffset           = other.hookPointOffset;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream = other.thenLocateActionFeatureStream;

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = other.locateActionExecuteTimes;

                    // fill stream
                    fillStream = other.fillStream;
                }

                SETUP_MOVE_METHOD(_HookPointFeatureInfo, other)
                {
                    //
                    // scan info
                    //

                    scanType = std::move(other.scanType);

                    // for 'ref'
                    refFeatureStream          = std::move(other.refFeatureStream);
                    refBackExtralInstruction  = std::move(other.refBackExtralInstruction);
                    refFrontExtralInstruction = std::move(other.refFrontExtralInstruction);
                    refFeatureStreamOffset    = other.refFeatureStreamOffset;

                    // for 'multiPushRef'
                    pushInstruction         = std::move(other.pushInstruction);
                    refFeatureStreams       = std::move(other.refFeatureStreams);
                    refFeatureStreamsOffset = std::move(other.refFeatureStreamsOffset);

                    // for 'instruction'
                    instructionFeatureStream = std::move(other.instructionFeatureStream);

                    //
                    // locate action
                    //

                    locateAction              = std::move(other.locateAction);
                    locateActionFeatureStream = std::move(other.locateActionFeatureStream);
                    locateActionRange         = other.locateActionRange;
                    hookPointOffset           = other.hookPointOffset;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream = std::move(other.thenLocateActionFeatureStream);

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = other.locateActionExecuteTimes;

                    // fill stream
                    fillStream = std::move(other.fillStream);
                }

                //
                // Method
                //

                void Reset()
                {
                    //
                    // scan info
                    //

                    scanType = "";

                    // for 'ref'
                    refFeatureStream.clear();
                    refBackExtralInstruction.clear();
                    refFrontExtralInstruction.clear();
                    refFeatureStreamOffset = 0;

                    // for 'multiPushRef'
                    pushInstruction.clear();
                    refFeatureStreams.clear();
                    refFeatureStreamsOffset.clear();

                    // for 'instruction'
                    instructionFeatureStream.clear();

                    //
                    // locate action
                    //

                    locateAction.clear();
                    locateActionFeatureStream.clear();
                    locateActionRange = 0;
                    hookPointOffset   = 0;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream.clear();

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = 0;

                    // fill stream
                    fillStream.clear();
                }

            } HookPointFeatureInfo, *PHookPointFeatureInfo;

            typedef struct _WxHookPointFeatures
            {
                std::string                                                   wxVersion;
                mutable std::unordered_map<std::string, HookPointFeatureInfo> mapApiFeature;

                //
                // constructor
                //

                _WxHookPointFeatures() = default;

                SETUP_COPY_METHOD(_WxHookPointFeatures, other)
                {
                    wxVersion     = other.wxVersion;
                    mapApiFeature = other.mapApiFeature;
                }

                SETUP_MOVE_METHOD(_WxHookPointFeatures, other)
                {
                    wxVersion     = std::move(other.wxVersion);
                    mapApiFeature = std::move(other.mapApiFeature);
                }

                //
                // Method
                //

                void Reset()
                {
                    wxVersion = "";
                    mapApiFeature.clear();
                }

                bool GetApiHookFeature(const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo) const;
                bool GetApiHookFeatureFillStream(const std::string& api, std::vector<uint8_t>& stream) const;

            } WxHookPointFeatures, *PWxHookPointFeatures;

            typedef struct _LocateTarget
            {
                wxbox::util::process::PROCESS_HANDLE hProcess;
                void*                                pModuleBaseAddr;
                ucpulong_t                           uModuleSize;
            } LocateTarget, *PLocateTarget;

            typedef struct _WxAPIHookPointVACollection
            {
                mutable std::unordered_map<std::string, ucpulong_t> collection;

                _WxAPIHookPointVACollection() = default;

                SETUP_COPY_METHOD(_WxAPIHookPointVACollection, other)
                {
                    collection = other.collection;
                }

                SETUP_MOVE_METHOD(_WxAPIHookPointVACollection, other)
                {
                    collection = std::move(other.collection);
                }

                void set(const std::string& api, ucpulong_t va)
                {
                    collection[api] = va;
                }

                ucpulong_t get(const std::string& api) const
                {
                    if (collection.find(api) == collection.end()) {
                        return 0;
                    }

                    return collection[api];
                }

            } WxAPIHookPointVACollection, *PWxAPIHookPointVACollection;

            PRAGMA(pack(push, 1))

            typedef struct _WxProfileItemOffset
            {
                ucpulong_t NickName;
                ucpulong_t WeChatNumber;
                ucpulong_t Wxid;
            } WxProfileItemOffset, *PWxProfileItemOffset;

            typedef struct _WxDataStructSupplement
            {
                WxProfileItemOffset profileItemOffset;
                ucpulong_t          logoutTriggerEventId;
                ucpulong_t          weChatContactHeaderItemOffset;
                ucpulong_t          weChatContactDataBeginOffset;
                ucpulong_t          weChatMessageStructureSize;
            } WxDataStructSupplement, *PWxDataStructSupplement;

            PRAGMA(pack(pop))

            typedef struct _WxApiFeatures
            {
                std::string                                             platform;
                std::string                                             featureFolderAbsPath;
                std::vector<wxbox::util::file::VersionNumber>           featureVersionList;
                std::unordered_map<std::string, WxAbsoluteHookInfo>     mapWxAbsoluteHookInfo;
                std::unordered_map<std::string, WxHookPointFeatures>    mapWxHookPointFeatures;
                std::unordered_map<std::string, WxDataStructSupplement> mapWxDataStructSupplement;

                //
                // constructor
                //

                _WxApiFeatures() = default;

                SETUP_COPY_METHOD(_WxApiFeatures, other)
                {
                    platform                  = other.platform;
                    featureFolderAbsPath      = other.featureFolderAbsPath;
                    featureVersionList        = other.featureVersionList;
                    mapWxAbsoluteHookInfo     = other.mapWxAbsoluteHookInfo;
                    mapWxHookPointFeatures    = other.mapWxHookPointFeatures;
                    mapWxDataStructSupplement = other.mapWxDataStructSupplement;
                }

                SETUP_MOVE_METHOD(_WxApiFeatures, other)
                {
                    platform                  = std::move(other.platform);
                    featureFolderAbsPath      = std::move(other.featureFolderAbsPath);
                    featureVersionList        = std::move(other.featureVersionList);
                    mapWxAbsoluteHookInfo     = std::move(other.mapWxAbsoluteHookInfo);
                    mapWxHookPointFeatures    = std::move(other.mapWxHookPointFeatures);
                    mapWxDataStructSupplement = std::move(other.mapWxDataStructSupplement);
                }

                //
                // Method
                //

                void Reset()
                {
                    platform             = "";
                    featureFolderAbsPath = "";
                    featureVersionList.clear();
                    mapWxAbsoluteHookInfo.clear();
                    mapWxHookPointFeatures.clear();
                    mapWxDataStructSupplement.clear();
                }

                bool        IsThisWxVersionExplicitLocated(const std::string& version);
                std::string FindSimilarVersion(const std::string& version);

                //
                // feature info
                //

                ucpulong_t GetAbsoluteHookPointRVA(const std::string& version, const std::string& api);
                bool       GetAbsoluteHookInfo(const std::string& version, WxAbsoluteHookInfo& wxAbsoluteHookInfo);

                bool GetHookPointFeature(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo);
                bool GetHookPointFeatures(const std::string& version, WxHookPointFeatures& wxHookPointFeatures);

                bool GetSimilarHookPointFeature(const std::string& version, const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo);
                bool GetSimilarHookPointFeatures(const std::string& version, WxHookPointFeatures& wxHookPointFeatures);

                //
                // hook point memory locate
                //

                ucpulong_t Locate(const LocateTarget& locateTarget, const std::string& version, const std::string& api);

                ucpulong_t AbsoluteLocate(const LocateTarget& locateTarget, const WxAbsoluteHookInfo& wxAbsoluteHookInfo, const std::string& api);
                ucpulong_t AbsoluteLocate(const LocateTarget& locateTarget, const std::string& version, const std::string& api);

                ucpulong_t FuzzyLocate(const LocateTarget& locateTarget, const HookPointFeatureInfo& hookPointFeatureInfo);
                ucpulong_t FuzzyLocate(const LocateTarget& locateTarget, const std::string& version, const std::string& api);

                //
                // collect hook point
                //

                bool Collect(const LocateTarget& locateTarget, const WxAbsoluteHookInfo& wxAbsoluteHookInfo, WxAPIHookPointVACollection& vaCollection);
                bool Collect(const LocateTarget& locateTarget, const WxHookPointFeatures& wxHookPointFeatures, WxAPIHookPointVACollection& vaCollection);
                bool Collect(const LocateTarget& locateTarget, const std::string& wxVersion, WxAPIHookPointVACollection& vaCollection);
                bool Collect(const wxbox::util::process::ProcessInfo& pi, const std::string& featureVersion, bool absoluteLocate, WxAPIHookPointVACollection& vaCollection);
                bool Collect(const wxbox::util::process::ProcessInfo& pi, const std::string& wxVersion, WxAPIHookPointVACollection& vaCollection);
                bool Collect(const wxbox::util::process::ProcessInfo& pi, WxAPIHookPointVACollection& vaCollection);

                //
                // obtain fill stream
                //

                bool ObtainFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream);
                bool ObtainAbsoluteFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream);
                bool ObtainFuzzyFillStream(const std::string& version, const std::string& api, std::vector<uint8_t>& stream);

                //
                // obtain datastructure supplement
                //

                bool ObtainDataStructureSupplement(const std::string& version, WxDataStructSupplement& wxDataStructSupplement);
                bool ObtainAbsoluteDataStructureSupplement(const std::string& version, WxDataStructSupplement& wxDataStructSupplement);
                bool ObtainFuzzyDataStructureSupplement(const std::string& version, WxDataStructSupplement& wxDataStructSupplement);

              private:
                bool                    Inner_LoadFeature(const std::string& version);
                WxAbsoluteHookInfo*     Inner_GetAbsoluteHookInfo(const std::string& version);
                WxHookPointFeatures*    Inner_GetHookPointFeatures(const std::string& version);
                WxHookPointFeatures*    Inner_GetSimilarHookPointFeatures(const std::string& version);
                WxDataStructSupplement* Inner_GetDataStructureSupplement(const std::string& version);

            } WxApiFeatures, *PWxApiFeatures;

            //
            // Function
            //

            bool                     ParseFeatureRepoList(const char* rawFeatureList, FeatureRepoList& repoFeatureList);
            std::vector<std::string> FeatureVersionList(const std::string& featuresPath);
            bool                     PreLoadFeatures(const std::string& featuresPath, WxApiFeatures& wxApiFeatures);
        }
    }
}

#endif  // #ifndef __WXBOX_CRACK_FEATURE_H