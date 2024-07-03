#pragma once

#include <type_traits>

#include "SIA/internals/types.hpp"

namespace sia
{
    namespace align_detail
    {
        struct align_info
        {
            size_t max;
            size_t min;
        };

        template <auto E>
        constexpr size_t max() noexcept
        {
            return E;
        }
        
        template <auto E0, auto E1, auto... Es>
        constexpr size_t max() noexcept
        {
            if constexpr (E0 >= E1)
            { return max<E0, Es...>(); }
            else { return max<E1, Es...>(); }
        }
    } // namespace align_detail
    
    template <typename... Ts>
    constexpr size_t max_align = align_detail::max<alignof(Ts)...>();
    constexpr align_detail::align_info align_info {alignof(largest_size_t), alignof(smallest_size_t)};
} // namespace sia
