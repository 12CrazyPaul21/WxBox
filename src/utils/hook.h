#ifndef __WXBOX_UTILS_HOOK_H
#define __WXBOX_UTILS_HOOK_H

namespace wxbox {
    namespace util {
        namespace hook {

            //
            // Function
            //

            bool InProcessHook(void* pfnOriginal, void* pfnNewEntry);
            bool InProcessDummyHook(void* pfnOriginal, void* pfnDummy);
            bool RevokeInProcessHook(void* pfnEntry);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_HOOK_H