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
        struct false_share_impl : align_wrapper<tuple_frame<Ts...>, std::hardware_constructive_interference_size>
        { };

        template <size_t N, typename T>
        struct wrapper : public T
        { using base_t = T::value_type; };

        template <typename... Ts>
        struct true_share_impl;

        template <typename... Ts, size_t... Seqs>
        struct true_share_impl<std::index_sequence<Seqs...>, Ts...> : wrapper<Seqs, align_wrapper<Ts, std::hardware_destructive_interference_size>>...
        {
            using type_index_t = type_index<wrapper<Seqs, align_wrapper<Ts, std::hardware_destructive_interference_size>>...>;
            // type_index_t::template at<N>::base_t*
            template <size_t N = 0>
            constexpr auto ptr() noexcept { return this->type_index_t::template at<N>::ptr(); }
            template <size_t N = 0>
            constexpr const auto ptr() const noexcept { return this->type_index_t::template at<N>::ptr(); }
            template <size_t N = 0>
            constexpr auto& ref() noexcept { return *(this->true_share_impl::ptr<N>()); }
            template <size_t N = 0>
            constexpr const auto& ref() const noexcept { return *(this->true_share_impl::ptr<N>()); }
        };
    } // namespace types_detail
    
    static_assert(std::is_trivially_copyable_v<std::thread::id> && std::is_standard_layout_v<std::thread::id>);
    using thread_id_t = unsigned_integer_t<sizeof(std::thread::id)>;

    template <typename T>
    using true_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename... Ts>
    using false_share  = types_detail::false_share_impl<Ts...>;
} // namespace sia
