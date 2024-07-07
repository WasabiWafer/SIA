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
        using contain_t = T;
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

        struct type_type { };
        struct false_type { };
    } // namespace type_container_detail
    
    template <typename... Ts>
    struct type_container : public type_sequence<Ts...>
    {
    private:
        using base_t = type_sequence<Ts...>;

        template <typename... Cs>
        constexpr auto remove_impl() noexcept
        {
            constexpr auto target_cond = [] <typename T> () { return is_same_any_v<T, Cs...>; };
            type_container copy{ };
            constexpr size_t run_count = sizeof...(Ts) - copy.count_if<0, sizeof...(Ts), target_cond>();
            
            using at_seq_t = std::make_index_sequence<sizeof...(Ts)>;
            using run_seq_t = std::make_index_sequence<run_count>;

            constexpr auto calc_pos =
            [] <size_t TargetCount, size_t At>
            (size_t& count, size_t& pos)
            {
                using pos_t = decltype(this->base_t::template at<At>());
                constexpr bool hit = target_cond.operator()<pos_t>();
                if (TargetCount == count)
                {
                    if (hit) { }
                    else { pos = At; ++count; }
                }
                else
                {
                    if (hit) { }
                    else { ++count; }
                }
            };

            constexpr auto get_pos =
            [] <size_t TargetCount, size_t... AtSeq>
            (std::index_sequence<AtSeq...>)
            {
                size_t count{ };
                size_t pos{ };
                ((calc_pos.operator()<TargetCount, AtSeq>(count, pos)), ...);
                return pos;
            };

            constexpr auto gen_type =
            [] <size_t TargetCount, size_t... AtSeq>
            (std::index_sequence<AtSeq...> arg) 
            {
                type_container copy{ };
                return copy.base_t::template at<get_pos.operator()<TargetCount>(arg)>().contain();
            };

            constexpr auto wrap =
            [] <size_t... RunSeq, size_t... AtSeq>
            (std::index_sequence<RunSeq...> arg0, std::index_sequence<AtSeq...> arg1)
            { return type_container<decltype(gen_type.operator()<RunSeq>(arg1))...>{ }; };

            return wrap(run_seq_t{ }, at_seq_t{ });
        }

    public:
        template <typename... Cs> using insert_t = type_container<Ts..., Cs...>;
        template <typename... Cs> constexpr insert_t<Cs...> insert() noexcept { return type_container<Ts..., Cs...>{ }; }
        
        template <size_t Begin, size_t End, auto Callable> requires (Begin < End)
        constexpr size_t count_if(const size_t count = 0) noexcept
        {   
            constexpr auto gen_seq = [] <auto... Seq> (std::index_sequence<Seq...>) { return std::index_sequence<(Seq + Begin)...>{ }; };
            using at_seq_t = decltype(gen_seq(std::make_index_sequence<End - Begin>{ }));
            constexpr auto calc =
            [] <size_t Pos>
            (size_t& count) constexpr noexcept
            {
                using pos_type = decltype(std::declval<type_container>().base_t::template at<Pos>());
                if (Callable.operator()<pos_type>()) { ++count; }
            };
            constexpr auto wrap =
            [] <auto... Seq>
            (std::index_sequence<Seq...>)
            {
                size_t count { };
                ((calc.operator()<Seq>(count)), ...);
                return count;
            };
            return wrap(at_seq_t{ });
        }

        template <size_t Begin, size_t End, auto Callable> requires (Begin < End)
        constexpr void for_each() noexcept
        {
            constexpr auto gen_seq = [] <auto... Seq> (std::index_sequence<Seq...>) { return std::index_sequence<(Seq + Begin)...>{ }; };
            using at_seq_t = decltype(gen_seq(std::make_index_sequence<End - Begin>{ }));
            constexpr auto run =
            [] <size_t At>
            ()
            {
                using pos_t = decltype(std::declval<type_container>().base_t::template at<At>());
                Callable.operator()<pos_t>();
            };
            constexpr auto wrap =
            [] <size_t... AtSeq>
            (std::index_sequence<AtSeq...>)
            {
                ((run.operator()<AtSeq>()), ...);
            };
            wrap(at_seq_t{ });
        }
        
        template <typename... Cs> requires (sizeof...(Cs) > 0)
        constexpr auto remove() noexcept { return remove_impl<Cs...>(); }
        template <size_t... Idxs> requires (sizeof...(Idxs) > 0)
        constexpr auto remove() noexcept { return remove_impl<decltype(base_t::template at<Idxs>())...>(); }
        template <typename... Cs> requires (sizeof...(Cs) == 0)
        constexpr auto remove() noexcept { return type_container{ }; }
    };
} // namespace sia