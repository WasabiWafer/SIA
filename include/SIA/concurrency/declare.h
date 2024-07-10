#pragma once

#include <print>

#include "SIA/internals/types.hpp"
#include "SIA/internals/tags.hpp"

#if defined(SIA_OS_WINDOW)
    // doc list
    // https://learn.microsoft.com/en-us/windows/win32/procthread/processes-and-threads
    // https://learn.microsoft.com/en-us/windows/win32/api/_processthreadsapi/
    // window include...
        #pragma comment(lib, "Kernel32")
    #include "Windows.h"

    //linux functions...
    template <typename... Ts> auto pthread_create(Ts...) { };
#elif defined(SIA_OS_LINUX)
    // doc list
    //linux include...



    //window functions...
    template <typename... Ts> auto CreateThread(Ts...) { };
#endif

namespace sia
{
    namespace thread_tag
    {
        enum class init : dword_t
        {
            #if defined(SIA_OS_WINDOW)
                run     = 0,
                suspend = CREATE_SUSPENDED
            #elif defined(SIA_OS_LINUX)

            #endif
        };

        enum class time : dword_t
        {
            #if defined(SIA_OS_WINDOW)
                inf = INFINITE
            #elif defined(SIA_OS_LINUX)
            #endif
        };

        enum class wait : dword_t
        {
            #if defined(SIA_OS_WINDOW)
                abandoned   = WAIT_ABANDONED,
                completion  = WAIT_IO_COMPLETION,
                object_0    = WAIT_OBJECT_0,
                timeout     = WAIT_TIMEOUT,
                failed      = WAIT_FAILED
            #elif defined(SIA_OS_LINUX)
            #endif
        };
    } // namespace thread_tag
} // namespace sia

namespace sia
{
    namespace thread_detail
    {
        template <typename Tuple_t, size_t... Ats>
        dword_t __stdcall thread_function(void* a_tuple) noexcept
        {
            const std::unique_ptr<Tuple_t> ptr(static_cast<Tuple_t*>(a_tuple));
            Tuple_t& tupl = *ptr.get();
            std::invoke(std::move(tupl.at<Ats>())...);
            return 0;
        }

        template <typename Tuple_t, size_t... Ats>
        constexpr auto get_thread_function(std::index_sequence<Ats...>) noexcept { return &thread_function<Tuple_t, Ats...>; }
    } // namespace thread_detail
} // namespace sia
