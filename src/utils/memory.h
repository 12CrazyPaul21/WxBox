#ifndef __WXBOX_UTILS_MEMORY_H
#define __WXBOX_UTILS_MEMORY_H

namespace wxbox {
    namespace util {
        namespace memory {

            //
            // Typedef
            //

            typedef struct _RemotePageInfo
            {
                static const ucpulong_t MIN_REMOTE_PAGE_SIZE = 0x1000;

                void*      addr;
                ucpulong_t size;
                void*      cursor;
                void*      end;
                ucpulong_t free;

                _RemotePageInfo()
                  : addr(nullptr)
                  , size(0)
                  , cursor(nullptr)
                  , end(nullptr)
                  , free(0)
                {
                }
            } RemotePageInfo, *PRemotePageInfo;

            typedef struct _RemoteWrittenMemoryInfo
            {
                void*      addr;
                ucpulong_t size;

                _RemoteWrittenMemoryInfo()
                  : addr(nullptr)
                  , size(0)
                {
                }
            } RemoteWrittenMemoryInfo, *PRemoteWrittenMemoryInfo;

            //
            // Function
            //

            void* AllocUnrestrictedMem(const size_t& count);
            void  FreeUnrestrictedMem(void* pMem);

            RemotePageInfo AllocPageToRemoteProcess(wxbox::util::process::PROCESS_HANDLE hProcess, ucpulong_t pageSize = RemotePageInfo::MIN_REMOTE_PAGE_SIZE, bool canExecute = false);
            bool           FreeRemoteProcessPage(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo);

            bool ReadMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pBaseAddress, uint8_t* pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesRead);
            bool WriteMemory(wxbox::util::process::PROCESS_HANDLE hProcess, void* pBaseAddress, const uint8_t* const pBuffer, ucpulong_t uSize, ucpulong_t* pNumberOfBytesWritten);

            RemoteWrittenMemoryInfo WriteByteStreamToProcess(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo, const uint8_t* const byteStream, ucpulong_t size);
            RemoteWrittenMemoryInfo WriteStringToProcess(wxbox::util::process::PROCESS_HANDLE hProcess, RemotePageInfo& pageInfo, const std::string& str);

            ucpulong_t ScanMemory(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize);
            ucpulong_t ScanMemoryRev(wxbox::util::process::PROCESS_HANDLE hProcess, const void* const pMemRevBegin, ucpulong_t uMemSize, const void* const pPattern, ucpulong_t uPatternSize);

            //
            // internal allocator, for avoid other suspended thread executing allocate cause deadlock
            //

            bool  init_internal_allocator();
            bool  deinit_internal_allocator();
            void* internal_malloc(size_t n);
            void  internal_free(void* p);

            template<class T>
            struct internal_allocator
            {
                typedef size_t   size_type;
                typedef T        value_type;
                typedef T*       pointer;
                typedef const T* const_pointer;
                typedef T&       reference;
                typedef const T& const_reference;

                internal_allocator()                                             = default;
                constexpr internal_allocator(const internal_allocator&) noexcept = default;

                template<class U>
                constexpr internal_allocator(const internal_allocator<U>&) noexcept
                {
                }

                template<class U>
                struct rebind
                {
                    using other = internal_allocator<U>;
                };

                pointer address(reference x) const
                {
                    return &x;
                }

                const_pointer address(const_reference x) const
                {
                    return &x;
                }

                size_type max_size() const noexcept
                {
#ifdef max
#undef max
#endif
                    return std::numeric_limits<std::size_t>::max() / sizeof(T);
                }

                pointer allocate(std::size_t n)
                {
                    if (n > max_size()) {
                        throw std::bad_array_new_length();
                    }

                    if (auto p = static_cast<pointer>(internal_malloc(n * sizeof(T)))) {
                        return p;
                    }

                    throw std::bad_alloc();
                }

                void deallocate(pointer p, std::size_t /*n*/) noexcept
                {
                    internal_free(p);
                }
            };
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_MEMORY_H