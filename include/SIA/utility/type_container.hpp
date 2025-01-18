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
                return typename decltype(mapper.template map<Idx>())::contain_t();
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
        using at_t = decltype(std::declval<base_t>().template at<Idx>());
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
            using bast_t = type_sequence<Ts...>;
        public:
            template <typename... Tys>
            constexpr auto insert_back(this auto&& self) noexcept { return type_container<Ts..., Tys...>(); }

            template <typename... Tys>
            constexpr auto insert_front(this auto&& self) noexcept { return type_container<Tys..., Ts...>(); }

            template <size_t Idx, typename... Tys>
            constexpr auto insert(this auto&& self) noexcept
            {
                std::make_index_sequence<Idx> front { };
                std::make_index_sequence<sizeof...(Ts) - Idx> back { };
                
                constexpr auto adder = [] <size_t Base, size_t... Indxs> (std::index_sequence<Indxs...>) constexpr noexcept -> auto
                    { return std::index_sequence<(Base + Indxs)...>(); };
                constexpr auto conv = [] <typename... Typs> (type_list<Typs...>) constexpr noexcept -> auto
                    { return type_container<Typs...>(); };
                constexpr auto comb = [] <typename... Typs0, typename... Typs1> (type_list<Typs0...>, type_list<Typs1...>) constexpr noexcept -> auto
                    { return type_list<Typs0..., Typs1...>(); };
                constexpr auto picker = [] <size_t... Indxs> (std::index_sequence<Indxs...>) constexpr noexcept -> auto
                    {
                        if constexpr (sizeof...(Indxs) == 0) { return type_list<>(); }
                        else { return type_list<decltype(self.template at<Indxs>())...>(); }
                    };
                constexpr auto run = [picker, comb, conv, front, back, adder] () constexpr noexcept -> auto
                    {
                        auto front_list = picker(front);
                        auto back_list = adder.template operator()<Idx>(back);
                        auto insert_front = comb(front_list, type_list<Tys...>());
                        auto insert_back = picker(back_list);
                        auto insert_type_list = comb(insert_front, insert_back);
                        auto ret = conv(insert_type_list);
                        return ret;
                    };
                return run();
            }

            template <auto Callable>
            constexpr size_t count_if(this auto&& self) noexcept
            {
                size_t ret { };
                constexpr auto run = [] <typename Typ> (size_t& arg) constexpr noexcept -> auto
                    {
                        if (Callable.template operator()<Typ>())
                        { ++arg; }
                        else
                        { }
                    };
                (run.template operator()<Ts>(ret), ...);
                return ret;
            }

            template <auto Callable>
            constexpr void for_each(this auto&& self) noexcept
            {
                (Callable.template operator()<Ts>(), ...);
            }

            template <size_t... Idxs>
            constexpr auto remove(this auto&& self) noexcept
            {
                std::make_index_sequence<sizeof...(Ts)> loop_seq { };

                constexpr auto count_remove_impl = [] (size_t arg0, size_t& arg1) constexpr noexcept -> auto
                    {
                        if ( ((arg0 == Idxs) || ...) ) { ++arg1; }
                        else { }
                    };
                constexpr auto count_remove = [count_remove_impl] <size_t... Indxs> (std::index_sequence<Indxs...>) constexpr noexcept -> auto
                    {
                        size_t ret { };
                        (count_remove_impl(Indxs, ret), ...);
                        return ret;
                    };

                std::make_index_sequence<sizeof...(Ts) - count_remove(loop_seq)> remove_run_seq { };

                constexpr auto is_remove_target = [] (size_t arg) constexpr noexcept -> auto
                    {
                        return ((arg == Idxs) || ...);
                    };
                constexpr auto get_n_th_idx_impl = [is_remove_target] <size_t Nth, size_t CurPos> (size_t& n_count, size_t& pos) constexpr noexcept -> auto
                    {
                        if (!is_remove_target(CurPos))
                        {
                            if (Nth == n_count) { pos = CurPos; }
                            ++n_count;
                        }
                    };
                constexpr auto get_n_th_idx = [get_n_th_idx_impl] <size_t Nth, size_t... Indxs> (std::index_sequence<Indxs...>) constexpr noexcept -> auto
                    {
                        size_t n_count { };
                        size_t pos { };
                        (get_n_th_idx_impl.template operator()<Nth, Indxs>(n_count, pos), ...);
                        return pos;
                    };
                constexpr auto gen_container_impl = [get_n_th_idx] <size_t Nth, size_t... Indxs> (std::index_sequence<Indxs...> l_seq) constexpr noexcept -> auto
                    {
                        constexpr type_container_impl copy { };
                        return copy.template at<get_n_th_idx.template operator()<Nth>(l_seq)>();
                    };
                constexpr auto gen_container = [gen_container_impl] <size_t... Indxs0, size_t... Indxs1> (std::index_sequence<Indxs0...> r_seq, std::index_sequence<Indxs1...> l_seq) constexpr noexcept -> auto
                    {
                        return type_container<decltype(gen_container_impl.template operator()<Indxs0>(l_seq))...>();
                    };

                return gen_container(remove_run_seq, loop_seq);
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
        using insert_back_t = decltype(std::declval<base_t>().template insert_back<Tys...>());
        template <typename... Tys>
        using insert_front_t = decltype(std::declval<base_t>().template insert_front<Tys...>());
        template <size_t Idx, typename... Tys>
        using insert_t = decltype(std::declval<base_t>().template insert<Idx, Tys...>());
        template <size_t... Idxs>
        using remove_t = decltype(std::declval<base_t>().template remove<Idxs...>());
    };
} // namespace sia
