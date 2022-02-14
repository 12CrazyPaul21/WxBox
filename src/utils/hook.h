#ifndef __WXBOX_UTILS_HOOK_H
#define __WXBOX_UTILS_HOOK_H

namespace wxbox {
    namespace util {
        namespace hook {

            //
            // Typedef
            //

            enum class HookPointType
            {
                Hook,
                Intercept,
                RelocateIntercept
            };

            typedef struct _HookMetaInfo
            {
                HookPointType        type;
                void*                entry;
                void*                actualEntry;
                void*                repeater;
                std::vector<uint8_t> originalEntryBackup;

                _HookMetaInfo()
                  : type(HookPointType::Hook)
                  , entry(nullptr)
                  , actualEntry(nullptr)
                  , repeater(nullptr)
                {
                }

                SETUP_COPY_METHOD(_HookMetaInfo, other)
                {
                    type                = other.type;
                    entry               = other.entry;
                    actualEntry         = other.actualEntry;
                    repeater            = other.repeater;
                    originalEntryBackup = other.originalEntryBackup;
                }

                SETUP_MOVE_METHOD(_HookMetaInfo, other)
                {
                    type                = other.type;
                    entry               = other.entry;
                    actualEntry         = other.actualEntry;
                    repeater            = other.repeater;
                    originalEntryBackup = std::move(other.originalEntryBackup);
                }

            } HookMetaInfo, *PHookMetaInfo;

            //
            // Function
            //

            bool InProcessHook(void* pfnOriginal, void* pfnNewEntry);
            bool InProcessDummyHook(void* pfnOriginal, void* pfnDummy);
            bool InProcessIntercept(void* pfnOriginal, void* pfnNewEntryOrStubEntry);

            bool RevokeInProcessHook(void* pfnEntry);
            bool ObtainHookMetaInfo(void* pfnOriginal, HookMetaInfo& hookMetaInfo);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_HOOK_H