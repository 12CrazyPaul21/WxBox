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
                size_t               repeaterSize;
                std::vector<uint8_t> originalEntryBackup;
                std::vector<uint8_t> hookOpcodes;
                bool                 isPre;
                bool                 delayedRelease;

                _HookMetaInfo()
                  : type(HookPointType::Hook)
                  , entry(nullptr)
                  , actualEntry(nullptr)
                  , repeater(nullptr)
                  , repeaterSize(0)
                  , isPre(false)
                  , delayedRelease(false)
                {
                }

                SETUP_COPY_METHOD(_HookMetaInfo, other)
                {
                    type                = other.type;
                    entry               = other.entry;
                    actualEntry         = other.actualEntry;
                    repeater            = other.repeater;
                    repeaterSize        = other.repeaterSize;
                    originalEntryBackup = other.originalEntryBackup;
                    hookOpcodes         = other.hookOpcodes;
                    isPre               = other.isPre;
                }

                SETUP_MOVE_METHOD(_HookMetaInfo, other)
                {
                    type                = other.type;
                    entry               = other.entry;
                    actualEntry         = other.actualEntry;
                    repeater            = other.repeater;
                    repeaterSize        = other.repeaterSize;
                    originalEntryBackup = std::move(other.originalEntryBackup);
                    hookOpcodes         = std::move(other.hookOpcodes);
                    isPre               = other.isPre;
                }

            } HookMetaInfo, *PHookMetaInfo;

            //
            // Function
            //

            bool InProcessHook(void* pfnOriginal, void* pfnNewEntry);
            bool InProcessDummyHook(void* pfnOriginal, void* pfnDummy);

            bool InProcessIntercept(void* pfnOriginal, void* pfnStubEntry);
            bool PreInProcessIntercept(void* pfnOriginal, void* pfnStubEntry);
            bool ExecuteInProcessIntercept(void* pfnOriginal);
            bool ReleasePreInProcessInterceptItem(void* pfnOriginal);

            bool RevokeInProcessHook(void* pfnEntry);
            bool ObtainHookMetaInfo(void* pfnOriginal, HookMetaInfo& hookMetaInfo);
        }
    }
}

#define WXBOX_INTERCEPT_STUB_RET_ADDR_OFFSET 0x0
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EFLAGS_OFFSET 0x04
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EDI_OFFSET 0x08
#define WXBOX_INTERCEPT_STUB_ORIGINAL_ESI_OFFSET 0x0C
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EBP_OFFSET 0x10
#define WXBOX_INTERCEPT_STUB_ORIGINAL_ESP_OFFSET 0x14
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EBX_OFFSET 0x18
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EDX_OFFSET 0x1C
#define WXBOX_INTERCEPT_STUB_ORIGINAL_ECX_OFFSET 0x20
#define WXBOX_INTERCEPT_STUB_ORIGINAL_EAX_OFFSET 0x24

#endif  // #ifndef __WXBOX_UTILS_HOOK_H