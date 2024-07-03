#pragma once

#include <type_traits>

#include "SIA/internals/types.hpp"

namespace sia
{
    namespace align_detail
    {
        template <size_t Size>
        constexpr auto calc_max_size_type() noexcept
        {
            if constexpr (std::is_same_v<void, unsigned_interger_t<Size>>)
            { return calc_max_size_type<Size * 2>(); }
            else { return unsigned_interger_t<Size/2>{ }; }
        }
    } // namespace align_detail
    
    constexpr size_t align_max = alignof(align_detail::calc_max_size_type<1>());

} // namespace sia
