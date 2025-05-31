#pragma once

#include <utility>

#include "SIA/internals/types.hpp"
#include "SIA/internals/define.hpp"

#include "SIA/utility/tools.hpp"

namespace sia
{
    template <typename... Ts>
    struct type_index;
    template <typename IndexSeq_t, typename TypeList_t>
    struct type_decomposition;

    namespace type_index_detail
    {
        SIA_MACRO_GEN_OVERLOAD(map_overload, map)

        template <typename T0, typename T1>
        struct select_impl;
        template <typename T, auto... Seqs>
        struct select_impl<T, entity_list<Seqs...>>
        { using type = type_index<typename T::template at<Seqs>...>; };

        template <typename T, typename List0, typename List1, typename... Ts>
        struct insert_at_impl;
        template <typename T, typename... Ts, size_t... Seq0, size_t... Seq1>
        struct insert_at_impl<T, entity_list<Seq0...>, entity_list<Seq1...>, Ts...>
        { using type = type_index<typename T::template at<Seq0>..., Ts..., typename T:: template at<Seq1>...>; };

        struct true_type { };
        struct false_type { };

        template <typename T>
        struct count_true_type_impl;
        template <typename... Ts>
        struct count_true_type_impl<type_index<Ts...>>
        {
            static constexpr size_t count_true_type() noexcept
            {
                constexpr auto lam = [] <typename T> (size_t& out) { if (std::is_same_v<true_type, T>) { ++out; } };
                size_t ret { };
                (lam.operator()<Ts>(ret), ...);
                return ret;
            };
            using result = entity<count_true_type()>;
        };
    } // namespace type_index_detail
    
    template <size_t Idx, typename T>
    struct type_token
    {
        template <size_t N>
            requires (Idx == N)
        constexpr T map(this auto&& self) noexcept;
    };

    template <size_t... Idxs, typename... Ts>
    requires (sizeof...(Ts) > 0)
    struct type_decomposition<std::index_sequence<Idxs...>, type_list<Ts...>>
    {
        static constexpr type_index_detail::map_overload mapper { type_token<Idxs, Ts>{ }... };
        template <size_t N>
        using at = decltype(mapper.map<N>());
        using back = at<sizeof...(Ts) - 1>;
        using front = at<0>;
    };

    template <size_t... Idxs, typename... Ts>
        requires (sizeof...(Ts) < 1)
    struct type_decomposition<std::index_sequence<Idxs...>, type_list<Ts...>>
    { };

    template <typename... Ts>
    struct type_index : type_decomposition<std::make_index_sequence<sizeof...(Ts)>, type_list<Ts...>>
    {
        template <size_t Begin, size_t End> requires (Begin < End)
        using select = type_index_detail::select_impl<type_index, bind_entity_list<[](size_t x){return x + Begin;}, make_entity_sequence<size_t, End - Begin>>>::type;
        template <typename Ty>
        using push_back = type_index<Ts..., Ty>;
        template <typename Ty>
        using push_front = type_index<Ty, Ts...>;
        template <size_t N>
        using pop_back_n = type_index_detail::select_impl<type_index, make_entity_sequence<size_t, sizeof...(Ts) - N>>::type;
        template <size_t N>
        using pop_front_n = type_index_detail::select_impl<type_index, bind_entity_list<[](size_t x){return x + N;}, make_entity_sequence<size_t, sizeof...(Ts) - N>>>::type;
        template <auto Func>
        using for_each = type_index<decltype(Func.operator()<Ts>())...>;
        template <size_t N, typename... Tys>
        using insert_at = type_index_detail::insert_at_impl<type_index, make_entity_sequence<size_t, N>,  bind_entity_list<[](size_t x){ return x + N;}, make_entity_sequence<size_t, sizeof...(Ts) - N>>, Tys...>::type;
        template <auto Func>
        using count_if = type_index_detail::count_true_type_impl<for_each<[]<typename T>(){ if constexpr (Func.operator()<T>() == true) { return type_index_detail::true_type{ };} else { return type_index_detail::false_type{ }; } }>>::result;
    };
} // namespace sia
