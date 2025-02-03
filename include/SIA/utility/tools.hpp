#pragma once

#include <type_traits>
#include <cassert>
#include <memory>

#include "SIA/internals/types.hpp"

#define assertm(exp, msg) assert((void(msg), exp))

namespace sia
{
    template <typename... Ts> struct overload : public Ts... { using Ts::operator()...; };
    template <typename... Ts> overload(Ts...) -> overload<Ts...>;

    template <auto Data>
    constexpr const auto& make_static() noexcept { return Data; }

    template <typename T, typename... Ts>
    struct is_same_all : public std::bool_constant<(std::is_same_v<T, Ts> && ...)> { };
    template <typename T>
    struct is_same_all<T> : public std::true_type { };
    template <typename T, typename... Ts>
    constexpr const bool is_same_all_v = is_same_all<T, Ts...>::value;

    template <typename T, typename... Ts>
    struct is_same_any : public std::bool_constant<(std::is_same_v<T, Ts> || ...)> { };
    template <typename T>
    struct is_same_any<T> : public std::false_type { };
    template <typename T, typename... Ts>
    constexpr const bool is_same_any_v = is_same_any<T, Ts...>::value;

    template <typename... Ts>
    struct type_list { using type = type_list; };
    template <auto... Es>
    struct entity_list { using type = entity_list; };
    
    enum class chunk_tag { heap, stack };

    template <typename T, size_t N, chunk_tag Tag = chunk_tag::stack, typename Allocator = std::allocator<T>>
    struct chunk;
    
    template <typename T, size_t N, typename Allocator>
    struct chunk<T, N, chunk_tag::heap, Allocator>
    {
        private:
        using allocator_traits_t = std::allocator_traits<Allocator>;
        Allocator m_alloc;
        public:
        T* m_bin;
        constexpr chunk(Allocator alloc = Allocator()) : m_alloc(alloc), m_bin(allocator_traits_t::allocate(m_alloc, N)) { allocator_traits_t::construct(m_alloc, m_bin); }
        ~chunk() { allocator_traits_t::deallocate(m_alloc, m_bin, N); }
    };

    template <typename T, size_t N>
    struct chunk<T, N, chunk_tag::stack> { T m_bin[N]; };
} // namespace sia
