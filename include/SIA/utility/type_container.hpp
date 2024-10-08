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
        constexpr size_t key(this auto&& self) noexcept { return Key; }
        constexpr T contain(this auto&& self) noexcept { return T{ }; }
    protected:
        template <size_t N>
        using map_t = decltype(std::declval<type_pair>().map<Key>());
        template <size_t N> requires (Key == N)
        constexpr type_pair map(this auto&& self) noexcept { return type_pair{ }; }
    };
} // namespace sia

//type_sequence
namespace sia
{
    namespace type_sequence_detail
    {
        template <class... Cls> struct overload : public Cls... { using Cls::map...; };
        template <class... Cls> overload(Cls...) -> overload<Cls...>;

        template <typename... Ts>
        struct type_sequence_impl;
        template <size_t... Seq, typename... Ts>
        struct type_sequence_impl<std::index_sequence<Seq...>, Ts...> : public type_pair<Seq, Ts>...
        {
        private:
            template <size_t Idx> using select_base_t = decltype(std::declval<overload<type_pair<Seq, Ts>...>>().map<Idx>());
        };
    } // namespace type_sequence_detail

    template <typename... Ts>
    struct type_sequence : public type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {
    private:
        using base_t = type_sequence_detail::type_sequence_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
    public:
        constexpr size_t size(this auto&& self) noexcept { return sizeof...(Ts); }
        template <size_t Idx> requires (Idx < sizeof...(Ts))
        constexpr auto at(this auto&& self) noexcept { return self.type_sequence::base_t::select_base_t<Idx>::template map<Idx>(); }
        template <size_t Idx> requires (Idx >= sizeof...(Ts))
        constexpr auto at(this auto&& self) noexcept { return nullptr; }
    };
} // namespace sia

//type_container
namespace sia
{
    template <typename... Ts>
    struct type_container : public type_sequence<Ts...>
    {
    private:
        using base_t = type_sequence<Ts...>;

        template <typename... Cs>
        constexpr auto remove_impl(this auto&& self) noexcept
        {
            constexpr auto target_cond = [] <typename T> () { return is_same_any_v<T, Cs...>; };
            type_container copy{ };
            constexpr size_t run_count = copy.size() - copy.count_if<0, copy.size(), target_cond>();
            
            using at_seq_t = std::make_index_sequence<sizeof...(Ts)>;
            using run_seq_t = std::make_index_sequence<run_count>;

            constexpr auto calc_pos =
            [] <size_t TargetCount, size_t At> (size_t& count, size_t& pos) constexpr noexcept
            {
                using pos_t = decltype(self.type_container::base_t::template at<At>());
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
            [] <size_t TargetCount, size_t... AtSeq> (std::index_sequence<AtSeq...>) constexpr noexcept
            {
                size_t count{ }; size_t pos{ };
                ((calc_pos.operator()<TargetCount, AtSeq>(count, pos)), ...);
                return pos;
            };

            constexpr auto gen_type =
            [] <size_t TargetCount, size_t... AtSeq> (std::index_sequence<AtSeq...> arg) constexpr noexcept
            {
                type_container copy{ };
                return copy.base_t::template at<get_pos.operator()<TargetCount>(arg)>().contain();
            };

            constexpr auto wrap =
            [] <size_t... RunSeq, size_t... AtSeq> (std::index_sequence<RunSeq...> arg0, std::index_sequence<AtSeq...> arg1) constexpr noexcept
            { return type_container<decltype(gen_type.operator()<RunSeq>(arg1))...>{ }; };

            return wrap(run_seq_t{ }, at_seq_t{ });
        }

    public:
        template <typename... Cs> using insert_t = type_container<Ts..., Cs...>;
        template <typename... Cs> constexpr insert_t<Cs...> insert() noexcept { return type_container<Ts..., Cs...>{ }; }

        template <size_t Begin, size_t End, auto Callable> requires (Begin < End)
        constexpr size_t count_if(this auto&& self, const size_t count = 0) noexcept
        {   
            constexpr auto gen_seq = [] <auto... Seq> (std::index_sequence<Seq...>) { return std::index_sequence<(Seq + Begin)...>{ }; };
            using at_seq_t = decltype(gen_seq(std::make_index_sequence<End - Begin>{ }));
            constexpr auto calc =
            [] <size_t Pos> (size_t& count) constexpr noexcept
            {
                using pos_t = decltype(self.type_container::base_t::template at<Pos>());
                if (Callable.operator()<pos_t>()) { ++count; }
            };
            constexpr auto wrap =
            [] <auto... Seq> (std::index_sequence<Seq...>) constexpr noexcept
            {
                size_t count { };
                ((calc.operator()<Seq>(count)), ...);
                return count;
            };
            return wrap(at_seq_t{ });
        }

        template <size_t Begin, size_t End, auto Callable> requires (Begin < End)
        constexpr void for_each(this auto&& self) noexcept
        {
            constexpr auto gen_seq = [] <auto... Seq> (std::index_sequence<Seq...>) { return std::index_sequence<(Seq + Begin)...>{ }; };
            using at_seq_t = decltype(gen_seq(std::make_index_sequence<End - Begin>{ }));
            constexpr auto run =
            [] <size_t At> () constexpr noexcept
            {
                using pos_t = decltype(self.type_container::base_t::template at<At>());
                Callable.operator()<pos_t>();
            };
            constexpr auto wrap = [] <size_t... AtSeq> (std::index_sequence<AtSeq...>) constexpr noexcept { ((run.operator()<AtSeq>()), ...); };
            wrap(at_seq_t{ });
        }
        
        template <typename... Cs> requires (sizeof...(Cs) > 0)
        constexpr auto remove(this auto&& self) noexcept { return self.remove_impl<Cs...>(); }
        template <size_t... Idxs> requires (sizeof...(Idxs) > 0)
        constexpr auto remove(this auto&& self) noexcept { return self.remove_impl<decltype(self.base_t::template at<Idxs>())...>(); }
        template <typename... Cs> requires (sizeof...(Cs) == 0)
        constexpr auto remove(this auto&& self) noexcept { return type_container{ }; }
    };
} // namespace sia