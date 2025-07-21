#pragma once

#include <thread>

#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/frame.hpp"

namespace sia
{
    namespace types_detail
    {
        template <typename... Ts>
        constexpr size_t get_total_size() noexcept
        {
            size_t ret { };
            ((ret += sizeof(Ts)), ...);
            return ret;
        }

        template <typename... Ts>
            requires (get_total_size<Ts...>() <= std::hardware_constructive_interference_size)
        struct true_share_impl : align_wrapper<tuple_frame<Ts...>, std::hardware_constructive_interference_size>
        { };
    } // namespace types_detail
    
    static_assert(std::is_trivially_copyable_v<std::thread::id> && std::is_standard_layout_v<std::thread::id>);
    using thread_id_t = unsigned_integer_t<sizeof(std::thread::id)>;

    template <typename T>
    using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename... Ts>
    using true_share  = types_detail::true_share_impl<Ts...>;
} // namespace sia
