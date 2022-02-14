#ifndef __WXBOX_UTILS_TRAITS_H
#define __WXBOX_UTILS_TRAITS_H

namespace wxbox {
    namespace util {
        namespace traits {

            //
            // Typedef
            //

            typedef struct _FunctionInfo
            {
                void*      addr;
                ucpulong_t size;

                _FunctionInfo()
                  : addr(nullptr)
                  , size(0)
                {
                }
            } FunctionInfo, *PFunctionInfo;

            //
            // Function
            //

            FunctionInfo FetchFunctionInfo(void* pFunc, void* pFuncEnd, bool ignoreFilledInt3 = false);
            void*        GetActualEntryAddress(void* pfnEntry);
            ucpulong_t   CpuOpcodeInstructionByteSize(void* pBeginOpcode);
        }
    }
}

//
// Macro
//

#define U_OBJ_CONSTRUCTOR(TYPE, MEMBER, MEMBER_TYPE) \
    if (type == TYPE) {                              \
        new (&MEMBER) MEMBER_TYPE();                 \
        return;                                      \
    }

#define U_OBJ_DESTRUCTOR(TYPE, MEMBER, DESTRUCTOR) \
    if (type == TYPE) {                            \
        MEMBER.~DESTRUCTOR();                      \
        return;                                    \
    }

#define U_OBJ_COPY(TYPE, MEMBER, MEMBER_TYPE, OTHER) \
    if (type == TYPE) {                              \
        new (&MEMBER) MEMBER_TYPE(OTHER.MEMBER);     \
        return;                                      \
    }

#define U_OBJ_MOVE(TYPE, MEMBER, MEMBER_TYPE, OTHER)        \
    if (type == TYPE) {                                     \
        new (&MEMBER) MEMBER_TYPE(std::move(OTHER.MEMBER)); \
        return;                                             \
    }

#define U_SCALAR_CONSTRUCTOR(TYPE, MEMBER, DEFAULT_VALUE) \
    if (type == TYPE) {                                   \
        MEMBER = DEFAULT_VALUE;                           \
        return;                                           \
    }

#define U_SCALAR_COPY(TYPE, MEMBER, OTHER) \
    if (type == TYPE) {                    \
        MEMBER = OTHER.MEMBER;             \
        return;                            \
    }

#define SETUP_COPY_METHOD(TYPE, OTHER_VAL_NAME) \
    TYPE(const TYPE& OTHER_VAL_NAME)            \
    {                                           \
        __copy(OTHER_VAL_NAME);                 \
    }                                           \
    TYPE& operator=(const TYPE& OTHER_VAL_NAME) \
    {                                           \
        __copy(OTHER_VAL_NAME);                 \
        return *this;                           \
    }                                           \
                                                \
    void __copy(const TYPE& OTHER_VAL_NAME)

#define SETUP_MOVE_METHOD(TYPE, OTHER_VAL_NAME) \
    TYPE(TYPE&& OTHER_VAL_NAME)                 \
    {                                           \
        __move(std::move(OTHER_VAL_NAME));      \
    }                                           \
    TYPE& operator=(TYPE&& OTHER_VAL_NAME)      \
    {                                           \
        __move(std::move(OTHER_VAL_NAME));      \
        return *this;                           \
    }                                           \
                                                \
    void __move(TYPE&& OTHER_VAL_NAME)

#define NAKED_FUNCTION_SEG_NAME ".nafunc"

#define BEGIN_NAKED_STD_FUNCTION(FUNC_NAME, ...)                                    \
    PRAGMA(code_seg(push, DEFINE_NAKED_STD_FUNCTION_BACK, NAKED_FUNCTION_SEG_NAME)) \
    void __declspec(naked) __stdcall FUNC_NAME(__VA_ARGS__)

#define __ASM() __asm

#define END_NAKED_STD_FUNCTION(FUNC_NAME)                                                             \
    void __declspec(naked) __stdcall FUNC_NAME##End()                                                 \
    {                                                                                                 \
        __ASM()                                                                                       \
        {                                                                                             \
            /*_emit 0x91*/                                                                            \
            ret                                                                                       \
        }                                                                                             \
    }                                                                                                 \
    wxbox::util::traits::FunctionInfo FUNC_NAME##Info(bool ignoreFilledInt3 = false)                  \
    {                                                                                                 \
        return wxbox::util::traits::FetchFunctionInfo(&FUNC_NAME, &FUNC_NAME##End, ignoreFilledInt3); \
    }                                                                                                 \
    PRAGMA(code_seg(pop, DEFINE_NAKED_STD_FUNCTION_BACK))

#endif  // #ifndef __WXBOX_UTILS_TRAITS_H