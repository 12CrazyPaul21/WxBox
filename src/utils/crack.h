#ifndef __WXBOX_UTILS_CRACK_H
#define __WXBOX_UTILS_CRACK_H

namespace wxbox {
    namespace util {
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

            bool OpenWxWithMultiBoxing(const wxbox::util::wx::WeChatEnvironmentInfo& wxEnvInfo, wxbox::util::feature::WxApiHookInfo& wxApiHookInfo, POpenWxWithMultiBoxingResult pResult = nullptr);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_CRACK_H