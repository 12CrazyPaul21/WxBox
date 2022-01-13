#ifndef __WXBOX_UTILS_INJECT_H
#define __WXBOX_UTILS_INJECT_H

namespace wxbox {
    namespace util {
        namespace inject {

            //
            // Typedef
            //

            PRAGMA(pack(push, 1))

            typedef struct _RemoteCallParameter
            {
                //
                // note: pFuncName must be a __cdecl function
                //

                char* pModuleName;
                char* pFuncName;
                void* pArg;
                void* pFuncGetProcAddress;
                void* pFuncGetModuleHandleA;

                _RemoteCallParameter()
                  : pModuleName(nullptr)
                  , pFuncName(nullptr)
                  , pArg(nullptr)
                  , pFuncGetProcAddress(nullptr)
                  , pFuncGetModuleHandleA(nullptr)
                {
                }
            } RemoteCallParameter, *PRemoteCallParameter;

            typedef enum class _MethodCallingParameterType
            {
                CpuWordLongScalarValue = 0,
                BufferPointer
            } MethodCallingParameterType;

            typedef struct _MethodCallingParameter
            {
                MethodCallingParameterType type;
                ucpulong_t                 value;
                ucpulong_t                 size;

                _MethodCallingParameter()
                  : type(MethodCallingParameterType::CpuWordLongScalarValue)
                  , value(0)
                  , size(sizeof(ucpulong_t))
                {
                }

                template<typename ScalarType>
                static _MethodCallingParameter BuildScalarValue(ScalarType value)
                {
                    static_assert(std::is_scalar<ScalarType>::value, "must be scalar type...");

                    _MethodCallingParameter parameter;
                    parameter.type  = MethodCallingParameterType::CpuWordLongScalarValue;
                    parameter.value = value;
                    parameter.size  = sizeof(ScalarType);

                    return parameter;
                }

                static _MethodCallingParameter BuildBufferValue(void* buffer, ucpulong_t bufferSize)
                {
                    _MethodCallingParameter parameter;
                    parameter.type  = MethodCallingParameterType::BufferPointer;
                    parameter.value = (ucpulong_t)buffer;
                    parameter.size  = bufferSize;

                    return parameter;
                }

            } MethodCallingParameter, *PMethodCallingParameter;

            PRAGMA(pack(pop))

            //
            // Function
            //

            bool InjectModuleToProcess(wxbox::util::process::PID pid, const std::string& modulePath, const std::string& entryMethod, PMethodCallingParameter parameter);
            bool UnInjectModuleFromProcess(wxbox::util::process::PID pid, const std::string& moduleName);

            bool CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);
            bool CallProcessModuleMethod(wxbox::util::process::PID pid, wxbox::util::memory::RemotePageInfo& dataPageInfo, wxbox::util::memory::RemotePageInfo& codePageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);

            bool CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, wxbox::util::memory::RemotePageInfo& dataPageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);
            bool CallProcessModuleMethod(wxbox::util::process::PID pid, wxbox::util::memory::RemotePageInfo& dataPageInfo, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);

            bool CallProcessModuleMethod(wxbox::util::process::PROCESS_HANDLE hProcess, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);
            bool CallProcessModuleMethod(wxbox::util::process::PID pid, const std::string& moduleName, const std::string& method, PMethodCallingParameter parameter);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_INJECT_H