#pragma once

#include <utility>
#include "SIA/utility/tools.hpp"

// entity_generator
namespace sia
{
    namespace entity_generator_detail
    {
        template <typename... Ts>
        struct concat_helper;
        template <auto... Es0, auto... Es1, typename... Ts>
        struct concat_helper<entity_list<Es0...>, entity_list<Es1...>, Ts...> { using type = typename concat_helper<entity_list<Es0..., Es1...>, Ts...>::type; };
        template <auto... Es>
        struct concat_helper<entity_list<Es...>> { using type = entity_list<Es...>; };

        template <auto Functor, typename SeqType, auto... Seq>
        constexpr auto generate_domain(std::integer_sequence<SeqType, Seq...>) noexcept
        {
            if constexpr (sizeof...(Seq) == 0) { return entity_list<>{ }; }
            return entity_list<Functor.operator()<Seq>()...>{ };
        }
        template <auto Functor, auto... Es>
        constexpr auto generate_range(entity_list<Es...> arg) noexcept { return entity_list<Functor.operator()<Es>()...>{ }; }
        template <auto... Functors, auto... Es>
        constexpr auto generate_range_applicative(entity_list<Es...> arg)
        {
            if constexpr (sizeof...(Functors) == 0) { return entity_list<>{ }; }
            return typename concat_helper<decltype(generate_range<Functors>(arg))...>::type { };
        }
    } // namespace entity_generator_detail
    
    template <typename IntegerSequenceType, auto Functor>
    using make_domain_sequence = decltype(entity_generator_detail::generate_domain<Functor>(IntegerSequenceType{ }));
    template <typename EntityListType, auto Functor, auto... Functors>
    using make_range_sequence = decltype(entity_generator_detail::generate_range_applicative<Functor, Functors...>(EntityListType{ }));
} // namespace sia

namespace sia
{
    template <auto Functor, typename EntityList>
    struct monadic;
    template <auto Functor, auto... Es>
    struct monadic<Functor, entity_list<Es...>>
    {
    private:
        using domain_t = entity_list<Es...>;
    public:
        template <auto... Ds>
        constexpr auto bind(entity_list<Ds...> arg) { return monadic<Functor, decltype(entity_generator_detail::generate_range<Functor>(arg))>{ }; }
        constexpr auto bind() noexcept { return monadic<Functor, decltype(entity_generator_detail::generate_range<Functor>(domain_t{ }))>{ }; }
        constexpr auto result() noexcept { return domain_t{ }; }
    };
} // namespace sia
