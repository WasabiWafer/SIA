#pragma once

#include <utility>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace type_pair_detail
    {
        struct fail_type{ };
    } // namespace type_pair_detail
    
    template <size_t Key, typename T>
    struct type_pair
    {
        using contain_t = T;
        constexpr size_t key(this auto&& self) noexcept { return Key; }

        template <size_t Idx>
            requires (Key == Idx)
        constexpr type_pair map(this auto&& self) noexcept { return self; }
    };

    namespace type_sequence_detail
    {
        template <typename... Ts> struct map_overload : public Ts... { using Ts::map...; };
        template <typename... Ts> map_overload(Ts...) -> map_overload<Ts...>;

        template <typename... Ts>
        struct type_sequence_impl;

        template <size_t... Keys, typename... Ts>
        struct type_sequence_impl<std::index_sequence<Keys...>, Ts...> : type_pair<Keys, Ts>...
        {
            template <size_t Idx>
            constexpr auto at(this auto&& self) noexcept
            {
                constexpr auto mapper = map_overload { type_pair<Keys, Ts>()... };
                return mapper.template map<Idx>();
            }
        };
    } // namespace type_sequence_detail
    
    template <typename... Ts>
    struct type_sequence : public type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {
    private:
        using base_t = type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
    public:
        template <size_t Idx>
        using at_t = decltype(std::declval<base_t>().template at<Idx>())::contain_t;
        constexpr size_t size(this auto&& self) noexcept { return sizeof...(Ts); }
    };

    template <typename... Ts>
    struct type_container;

    namespace type_container_detail
    {
        template <typename... Ts>
        struct type_container_impl : type_sequence<Ts...>
        {
        private:
            template <size_t Idx, typename... Tys>
            consteval auto insert_impl(this auto&& self) noexcept
            {
                
            }
        public:
            template <typename... Tys>
            constexpr auto push_back(this auto&& self) noexcept { return type_container<Ts..., Tys...>(); }

            template <typename... Tys>
            constexpr auto push_front(this auto&& self) noexcept { return type_container<Tys..., Ts...>(); }

            template <size_t Idx, typename... Tys>
            constexpr auto insert(this auto&& self) noexcept
            {
                
            }
        };
    } // namespace type_container_detail
    
    template <typename... Ts>
    struct type_container : type_container_detail::type_container_impl<Ts...>
    {
    private:
        using base_t = type_container_detail::type_container_impl<Ts...>;

    public:
        template <typename... Tys>
        using push_back_t = decltype(std::declval<base_t>().template push_back<Tys...>());
        template <typename... Tys>
        using push_front_t = decltype(std::declval<base_t>().template push_front<Tys...>());
        template <size_t Idx, typename... Tys>
        using insert_t = decltype(std::declval<base_t>().template insert<Idx, Tys...>());
        // remove, insert, for_each, count_if
    };
} // namespace sia
