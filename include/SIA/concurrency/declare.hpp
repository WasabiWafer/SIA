#pragma once

#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/internals/tags.hpp"

#define empty_struct(struct_name)                       struct struct_name{ };
#define empty_type_function(func_name, return_type)     template<typename...Ts> constexpr return_type func_name(Ts...) noexcept { return return_type{ }; }
#define empty_pointer_function(func_name)               template<typename...Ts> constexpr auto func_name(Ts...) noexcept { return nullptr; }

#if defined(SIA_OS_WINDOW)
    // doc list
    // https://learn.microsoft.com/en-us/windows/win32/procthread/processes-and-threads
    // https://learn.microsoft.com/en-us/windows/win32/api/_processthreadsapi/
    // window include...
    #pragma comment(lib, "Kernel32")
    #include "Windows.h"
    //linux functions...
    empty_pointer_function(pthread_create)
#elif defined(SIA_OS_LINUX)
    // doc list
    //linux include...
    //window functions...
    empty_pointer_function(CreateThread)
    empty_pointer_function(SuspendThread)
    empty_pointer_function(ResumeThread)
    empty_type_function(CloseHandle, dword_t)
    empty_type_function(WaitForSingleObjectEx, dword_t)
    empty_type_function(GetCurrentThreadId, dword_t)
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
        dword_t __stdcall generic_thread_function(void* a_tuple) noexcept
        {
            const std::unique_ptr<Tuple_t> ptr(static_cast<Tuple_t*>(a_tuple));
            Tuple_t& tupl = *ptr.get();
            std::invoke(std::move(std::get<Ats>(tupl))...);
            return 0;
        }

        template <typename Tuple_t, size_t... Ats>
        auto __stdcall generic_function(void* a_tuple) noexcept
        {
            const std::unique_ptr<Tuple_t> ptr(static_cast<Tuple_t*>(a_tuple));
            Tuple_t& tupl = *ptr.get();
            return std::invoke(std::move(std::get<Ats>(tupl))...);
        }

        template <typename Tuple_t, size_t... Ats>
        constexpr auto get_generic_thread_function(std::index_sequence<Ats...>) noexcept { return &generic_thread_function<Tuple_t, Ats...>; }

        template <typename Tuple_t, size_t... Ats>
        constexpr auto get_generic_function(std::index_sequence<Ats...>) noexcept { return &generic_function<Tuple_t, Ats...>; }
    } // namespace thread_detail
} // namespace sia

#undef empty_struct
#undef empty_type_function
#undef empty_pointer_function