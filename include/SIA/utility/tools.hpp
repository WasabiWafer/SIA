#pragma once

#include <type_traits>
#include <cassert>
#include <memory>
#include <bit>

#include "SIA/internals/types.hpp"
#include "SIA/internals/define.hpp"
#include "SIA/utility/compressed_pair.hpp"

#define assertm(exp, msg) assert((void(msg), exp))

namespace sia
{
    template <typename... Ts> struct overload : public Ts... { using Ts::operator()...; };
    template <typename... Ts> overload(Ts...) -> overload<Ts...>;

    template <auto Data>
    constexpr const auto& make_static() noexcept { return Data; }

    template <typename To, typename From>
    [[nodiscard]]
    constexpr To type_cast(const From& arg) noexcept { return std::bit_cast<To>(arg); }

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
    
    template <typename T, size_t N, tags::memory_locations Tag = tags::memory_locations::stack, typename Allocator = std::allocator<T>>
    struct chunk;
    
    template <typename T, size_t N, typename Allocator>
    struct chunk<T, N, tags::memory_locations::heap, Allocator>
    {
        private:
        using allocator_traits_t = std::allocator_traits<Allocator>;
        compressed_pair<Allocator, T*> m_compair;
        public:
        constexpr chunk(const Allocator& alloc = Allocator()) noexcept(noexcept(Allocator(alloc)) && noexcept(allocator_traits_t::allocate(alloc, N)))
            : m_compair(splits::one_v, alloc)
        { m_compair.second() = allocator_traits_t::allocate(m_compair.first(), N);}
        ~chunk() noexcept(noexcept(allocator_traits_t::deallocate(m_compair.first(), m_compair.second(), N)))
        { allocator_traits_t::deallocate(m_compair.first(), m_compair.second(), N); }

        constexpr T* ptr() noexcept { return m_compair.second(); }
        constexpr const T* ptr() const noexcept { return m_compair.second(); }
    };

    template <typename T, size_t N>
    struct chunk<T, N, tags::memory_locations::stack>
    {
    private:
        T m_bin[N];
    public:
        constexpr T* ptr() noexcept { return m_bin; }
        constexpr const T* ptr() const noexcept { return m_bin; }
    };
} // namespace sia
