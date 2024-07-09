#pragma once

#include <type_traits>

#include "SIA/concurrency/declare.h"
#include "SIA/internals/types.hpp"
#include "SIA/internals/tags.hpp"
#include "SIA/utility/constant_tag.hpp"

namespace sia
{
    namespace thread_tag
    {
        enum class policy { run, suspend };
    } // namespace thread_tag
    
} // namespace sia


namespace sia
{
    namespace thread_detail
    {
        int __stdcall thread_function() noexcept
        {

        }
    } // namespace thread_detail

    template <auto... Es>
    struct thread;
    template <>
    struct thread<system::os> : private constant_tag<system::os>
    {
        using tag_t = constant_tag<system::os>;
        void* handle;
        dword_t id;

        thread() = default;
        template <typename F, typename... Ts>
        thread(F&& func, Ts&&... args)
        {

        }
        thread(thread&& arg)
        {
            // to do
        }

        template <typename F, typename... Ts>
        void init(F&& func, Ts&&... args)
        {
            if constexpr (tag_t::query(tags::os::window))
            {
                // handle = reinterpret_cast<void*>(CreateThread(NULL, NULL, func, ));
            }
            else
            {

            }
        }

        

    };
} // namespace sia
