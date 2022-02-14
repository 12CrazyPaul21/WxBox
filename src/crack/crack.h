#ifndef __WXBOX_CRACK_H
#define __WXBOX_CRACK_H

namespace wxbox {
    namespace crack {

        //
        // Typedef
        //

        typedef struct _OpenWxWithMultiBoxingResult
        {
            wxbox::util::process::PID pid;
            void*                     pModuleBaseAddr;
            ucpulong_t                uModuleSize;
        } OpenWxWithMultiBoxingResult, *POpenWxWithMultiBoxingResult;

        //
        // Function
        //

        bool OpenWxWithMultiBoxing(const wxbox::crack::wx::WeChatEnvironmentInfo& wxEnvInfo, wxbox::crack::feature::WxApiFeatures& wxApiFeatures, POpenWxWithMultiBoxingResult pResult = nullptr);
    }
}

#endif  // #ifndef __WXBOX_CRACK_H