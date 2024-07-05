#pragma once

#include <type_traits>
#include <utility>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"

//type_pair
namespace sia
{
    template <size_t Key, typename T>
    struct type_pair
    {
    public:
        using container_t = T;
        constexpr size_t key() noexcept { return Key; }
        constexpr T contain() noexcept { return T{ }; }
    protected:
        template <size_t N>
        using hash_t = decltype(std::declval<type_pair>().hash<Key>());
        template <size_t N> requires (Key == N)
        constexpr type_pair hash() noexcept { return type_pair{ }; }
    };
} // namespace sia

//type_sequence
namespace sia
{
    namespace type_sequence_detail
    {
        template <class... Cls> struct overload : public Cls... { using Cls::hash...; };
        template <class... Cls> overload(Cls...) -> overload<Cls...>;

        template <typename... Ts>
        struct type_sequence_impl;
        template <size_t... Seq, typename... Ts>
        struct type_sequence_impl<std::index_sequence<Seq...>, Ts...> : public type_pair<Seq, Ts>...
        {
        private:
            template <size_t Idx> using select_base_t = decltype(std::declval<overload<type_pair<Seq, Ts>...>>().hash<Idx>());
        };
    } // namespace type_sequence_detail

    template <typename... Ts>
    struct type_sequence : public type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {
    private:
        using base_t = type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
    public:
        constexpr size_t size() noexcept { return sizeof...(Ts); }
        template <size_t Idx> requires (Idx < sizeof...(Ts))
        constexpr auto at() noexcept { return base_t::select_base_t<Idx>::template hash<Idx>(); }
        template <size_t Idx> requires (Idx >= sizeof...(Ts))
        constexpr auto at() noexcept { return nullptr; }
    };
} // namespace sia

//type_container
namespace sia
{
    namespace type_container_detail
    {
        template <typename... Ts>
        struct concat_helper;
        template <typename T, typename... Ts>
        struct concat_helper<T, type_list<Ts...>> { using type = type_list<T, Ts...>; };
    } // namespace type_container_detail
    
    template <typename... Ts>
    struct type_container : public type_sequence<Ts...>
    {
    private:
        using base_t = type_sequence<Ts...>;
        template <size_t Hit, size_t Pos, auto Callable>
        constexpr auto shrink_impl() noexcept
        {
            using pos_type = decltype(base_t::template at<Pos>());
            using ret_type = decltype(base_t::template at<Pos>().contain());
            constexpr bool flag = Callable.operator()<pos_type>();
            if constexpr (Hit == 0)
            {
                if constexpr (flag) { return shrink_impl<Hit, Pos + 1, Callable>(); }
                else { return type_list<ret_type>{ }; }
            }
            else
            {
                if constexpr (flag) { return shrink_impl<Hit, Pos + 1, Callable>(); }
                else { return typename type_container_detail::concat_helper<ret_type, decltype(shrink_impl<Hit - 1, Pos + 1, Callable>())>::type{ }; }
            }
        }

        template <typename... Ts>
        constexpr auto gen_ret(type_list<Ts...>) noexcept { return type_container<Ts...>{ }; }
        
        template <auto Callable, size_t Count>
        constexpr auto shrink() noexcept
        {
            using list_t = decltype(shrink_impl<Count - 1, 0, Callable>());
            return gen_ret(list_t{ });
        }

        template <typename... Cs>
        constexpr auto remove_impl() noexcept
        {
            constexpr auto target_cond = [] <typename T> () { return is_same_any_v<T, Cs...>; };
            type_container copy{ };
            constexpr size_t run_count = copy.size() - copy.count_if<0, copy.size(), target_cond>();
            return shrink<target_cond, run_count>();
        }

        template <typename... Cs> requires (sizeof...(Ts) == 0)
        constexpr auto remove_impl() noexcept
        {
            return type_container{ };
        }

    public:
        template <typename... Cs> using insert_t = type_container<Ts..., Cs...>;
        template <typename... Cs> constexpr insert_t<Cs...> insert() noexcept { return type_container<Ts..., Cs...>{ }; }

        template <size_t Begin, size_t End, auto Callable>
        constexpr size_t count_if(const size_t count = 0) noexcept
        {
            
            using pos_type = decltype(base_t::template at<Begin>());
            constexpr bool flag = Callable.operator()<pos_type>();
            if constexpr (Begin + 1 >= End)
            {
                if constexpr (flag) { return count + 1; }
                else                { return count; }
            }
            else
            {
                if constexpr (flag) { return type_container::template count_if<Begin + 1, End, Callable>(count + 1); }
                else                { return type_container::template count_if<Begin + 1, End, Callable>(count); }
            }
        }

        template <size_t Begin, size_t End, auto Callable>
        constexpr void for_each() noexcept
        {
            
        }
        
        template <typename... Cs>
        constexpr auto remove() noexcept { return remove_impl<Cs...>(); }
        template <size_t... Idxs>
        constexpr auto remove() noexcept { return remove_impl<decltype(base_t::template at<Idxs>())...>(); }
    };
} // namespace sia