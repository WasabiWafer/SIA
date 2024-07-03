#pragma once

#include <type_traits>
#include <utility>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/memory/constant_allocator.hpp"

//type_pair
namespace sia
{
    template <size_t Key, typename T>
    struct type_pair
    {
    public:
        constexpr auto key() noexcept { return Key; }
        constexpr auto contain() noexcept { return T{ }; }
    protected:
        template <size_t N> requires (Key == N)
        constexpr auto hash() noexcept { return *this; }
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
        constexpr auto size() noexcept { return sizeof...(Ts); }
        template <size_t Idx> requires (Idx < sizeof...(Ts))
        constexpr auto at() noexcept { return this->base_t::select_base_t<Idx>::hash<Idx>(); }
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
        template <size_t Hit, size_t Pos, auto Callable>
        constexpr auto shrink_impl() noexcept
        {
            using pos_type = decltype(this->at<Pos>());
            using ret_type = decltype(this->at<Pos>().contain());
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

    public:
        template <typename... Ts1> constexpr auto insert() noexcept { return type_container<Ts..., Ts1...>{ }; }
        template <typename... Ts1> using insert_t = type_container<Ts..., Ts1...>;

        template <size_t Begin, size_t End, auto Callable>
        constexpr size_t count_if(size_t count = 0) noexcept
        {
            using pos_type = decltype(this->at<Begin>());
            constexpr bool flag = Callable.operator()<pos_type>();
            if constexpr (Begin + 1 >= End)
            {
                if constexpr (flag) { return count + 1; }
                else                { return count; }
            }
            else
            {
                if constexpr (flag) { return count_if<Begin + 1, End, Callable>(count + 1); }
                else                { return count_if<Begin + 1, End, Callable>(count); }
            }
        }
        
        template <typename... Cs>
        constexpr auto remove() noexcept { return remove_impl<Cs...>(); }
        template <size_t... Idxs>
        constexpr auto remove() noexcept { return remove_impl<decltype(this->at<Idxs>())...>(); }
    };
} // namespace sia