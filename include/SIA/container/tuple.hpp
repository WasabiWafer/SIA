#pragma once

#include "SIA/utility/type_container.hpp"

// tuple_data
namespace sia
{
    template <size_t Key, typename T> struct tuple_data { T data; };
} // namespace sia

// tuple
namespace sia
{
    namespace tuple_detail
    {
        template <typename... Ts>
        struct tuple_impl;
        template <size_t... Seq, typename... Ts>
        struct tuple_impl<std::index_sequence<Seq...>, Ts...> : public tuple_data<Seq, Ts>...
        {
            using tycon = type_container<tuple_data<Seq, Ts>...>;
            template <size_t Pos>
            constexpr auto& at() noexcept { return decltype(std::declval<tycon>().at<Pos>().contain())::data; }
        };
    } // namespace tuple_detail
    
    template <typename... Ts>
    struct tuple : public tuple_detail::tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...> { };
    
    template <typename... Ts>
    tuple(Ts...) -> tuple<Ts...>;
} // namespace sia
