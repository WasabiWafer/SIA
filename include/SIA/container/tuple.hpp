#pragma once

#include "SIA/utility/type_container.hpp"

// tuple
namespace sia
{
    namespace tuple_detail
    {
        template <size_t Key, typename T> struct tuple_data { T data; };
        
        template <typename... Ts>
        struct tuple_impl;
        template <size_t... Seq, typename... Ts>
        struct tuple_impl<std::index_sequence<Seq...>, Ts...> : public tuple_data<Seq, Ts>...
        {
            using tycon = type_container<tuple_data<Seq, Ts>...>;
            template <size_t Pos>
            constexpr auto& at(this auto&& self) noexcept { return self.decltype(std::declval<tycon>().at<Pos>().contain())::data; }

            // template <typename... Cs, size_t... Seq>
            // constexpr tuple_impl(std::index_sequence<Seq...>, Cs&&... args) noexcept : { }
        };
    } // namespace tuple_detail
    
    template <typename... Ts>
    struct tuple : public tuple_detail::tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {
        using base_t = tuple_detail::tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
        template <typename... Cs>
        constexpr tuple(Cs&&... args) noexcept : base_t{std::forward<Cs>(args)...} { }
    };
    
    template <typename... Ts>
    tuple(Ts...) -> tuple<Ts...>;
} // namespace sia
