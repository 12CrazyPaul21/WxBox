#ifndef __WXBOX_UTILS_FEATURE_H
#define __WXBOX_UTILS_FEATURE_H

namespace wxbox {
    namespace util {
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
                "WXSendFileMessage"};

            //
            // Typedef
            //

            typedef struct _WxAbsoluteHookInfo
            {
                std::string                               wxVersion;
                std::unordered_map<std::string, uint32_t> mapApiRva;

                //
                // constructor
                //

                _WxAbsoluteHookInfo()
                {
                }

                SETUP_COPY_METHOD(_WxAbsoluteHookInfo, other)
                {
                    wxVersion = other.wxVersion;
                    mapApiRva = other.mapApiRva;
                }

                SETUP_MOVE_METHOD(_WxAbsoluteHookInfo, other)
                {
                    wxVersion = std::move(other.wxVersion);
                    mapApiRva = std::move(other.mapApiRva);
                }

                //
                // Method
                //

                void Reset()
                {
                    wxVersion = "";
                    mapApiRva.clear();
                }

                uint32_t GetApiRva(const std::string& api);

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

                // for 'multiPushRef' scan type
                std::vector<uint8_t>              pushInstruction;
                std::vector<std::vector<uint8_t>> refFeatureStreams;

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
                int                  hookPointOffset;

                // for 'backThenFront' locate action
                std::vector<uint8_t> thenLocateActionFeatureStream;

                // for 'backMultiTimes' locate action
                int locateActionExecuteTimes;

                //
                // constructor
                //

                _HookPointFeatureInfo()
                  : hookPointOffset(0)
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

                    // for 'multiPushRef'
                    pushInstruction   = other.pushInstruction;
                    refFeatureStreams = other.refFeatureStreams;

                    // for 'instruction'
                    instructionFeatureStream = other.instructionFeatureStream;

                    //
                    // locate action
                    //

                    locateAction              = other.locateAction;
                    locateActionFeatureStream = other.locateActionFeatureStream;
                    hookPointOffset           = other.hookPointOffset;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream = other.thenLocateActionFeatureStream;

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = other.locateActionExecuteTimes;
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

                    // for 'multiPushRef'
                    pushInstruction   = std::move(other.pushInstruction);
                    refFeatureStreams = std::move(other.refFeatureStreams);

                    // for 'instruction'
                    instructionFeatureStream = std::move(other.instructionFeatureStream);

                    //
                    // locate action
                    //

                    locateAction              = std::move(other.locateAction);
                    locateActionFeatureStream = std::move(other.locateActionFeatureStream);
                    hookPointOffset           = other.hookPointOffset;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream = std::move(other.thenLocateActionFeatureStream);

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = other.locateActionExecuteTimes;
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

                    // for 'multiPushRef'
                    pushInstruction.clear();
                    refFeatureStreams.clear();

                    // for 'instruction'
                    instructionFeatureStream.clear();

                    //
                    // locate action
                    //

                    locateAction.clear();
                    locateActionFeatureStream.clear();
                    hookPointOffset = 0;

                    // for 'backThenFront'
                    thenLocateActionFeatureStream.clear();

                    // for 'backMultiTimes'
                    locateActionExecuteTimes = 0;
                }

            } HookPointFeatureInfo, *PHookPointFeatureInfo;

            typedef struct _WxHookPointFeatures
            {
                std::string                                           wxVersion;
                std::unordered_map<std::string, HookPointFeatureInfo> mapApiFeature;

                //
                // constructor
                //

                _WxHookPointFeatures()
                {
                }

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

                bool GetApiHookFeature(const std::string& api, HookPointFeatureInfo& hookPointFeatureInfo);

            } WxHookPointFeatures, *PWxHookPointFeatures;

            typedef struct _WxApiHookInfo
            {
                std::string                                          platform;
                std::string                                          featureFileAbsPath;
                std::unordered_map<std::string, WxAbsoluteHookInfo>  mapWxAbsoluteHookInfo;
                std::unordered_map<std::string, WxHookPointFeatures> mapWxHookPointFeatures;

                //
                // constructor
                //

                _WxApiHookInfo()
                {
                }

                SETUP_COPY_METHOD(_WxApiHookInfo, other)
                {
                    platform               = other.platform;
                    featureFileAbsPath     = other.featureFileAbsPath;
                    mapWxAbsoluteHookInfo  = other.mapWxAbsoluteHookInfo;
                    mapWxHookPointFeatures = other.mapWxHookPointFeatures;
                }

                SETUP_MOVE_METHOD(_WxApiHookInfo, other)
                {
                    platform               = std::move(other.platform);
                    featureFileAbsPath     = std::move(other.featureFileAbsPath);
                    mapWxAbsoluteHookInfo  = std::move(other.mapWxAbsoluteHookInfo);
                    mapWxHookPointFeatures = std::move(other.mapWxHookPointFeatures);
                }

                //
                // Method
                //

                void Reset()
                {
                    platform           = "";
                    featureFileAbsPath = "";
                    mapWxAbsoluteHookInfo.clear();
                    mapWxHookPointFeatures.clear();
                }

                bool GetWxAbsoluteHookInfoWithVersion(const std::string& version, WxAbsoluteHookInfo& wxAbsoluteHookInfo);
                bool GetWxHookPointFeaturesWithVersion(const std::string& version, WxHookPointFeatures& wxHookPointFeatures);
                bool GetWxHookPointFeaturesWithSimilarVersion(const std::string& version, WxHookPointFeatures& wxHookPointFeatures);

            } WxApiHookInfo, *PWxApiHookInfo;

            //
            // Function
            //

            bool UnwindFeatureConf(const std::string& confPath, WxApiHookInfo& wxApiHookInfo);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_FEATURE_H