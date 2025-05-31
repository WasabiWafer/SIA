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

    #define SIA_MACRO_GEN_OVERLOAD(STRUCT_NAME, TARGET_FUNC_NAME) \
    template <typename... Ts> struct STRUCT_NAME : public Ts... { using Ts::TARGET_FUNC_NAME...; };\
    template <typename... Ts> STRUCT_NAME(Ts...) -> STRUCT_NAME<Ts...>;

    // template <auto Data>
    // constexpr const auto& make_static() noexcept { return Data; }

    template <typename To, typename From>
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
    
    template <auto Func, typename... Args>
    struct is_nothrow_function : std::bool_constant<requires() { {Func(Args()...)} noexcept; }> { };
    template <auto Func, typename... Args>
    constexpr const bool is_nothrow_function_v = is_nothrow_function<Func, Args...>::value;

    template <typename Class, auto Func, typename... Args>
    struct is_nothrow_member_function : std::bool_constant<requires(Class c) { {(c.*Func)(Args()...)} noexcept; }> { };
    template <typename Class, auto Func, typename... Args>
    constexpr const bool is_nothrow_member_function_v = is_nothrow_member_function<Class, Func, Args...>::value;

    template <auto E>
    struct entity
    {
        using type = decltype(E);
        static constexpr const auto& value = E;
        constexpr const auto& operator()(this auto&& self) noexcept { return value; }
    };

    template <typename... Ts>
    struct type_list { using type = type_list; };
    template <auto... Es>
    struct entity_list { using type = entity_list; };

    namespace tools_detail
    {
        template <typename T0, typename T1>
        struct concat_entity_list_impl;

        template <auto... Es0, auto... Es1>
        struct concat_entity_list_impl<entity_list<Es0...>, entity_list<Es1...>>
        { using type = entity_list<Es0..., Es1...>; };

        template <typename T0, typename T1>
        struct make_entity_sequence_impl;

        template <typename T, size_t... Seqs>
        struct make_entity_sequence_impl<T, std::integer_sequence<size_t, Seqs...>>
        { using type = entity_list<T{Seqs}...>; };

        template <auto Func, typename EntityList>
        struct bind_entity_list_impl;

        template <auto Func, auto... Es>
        struct bind_entity_list_impl<Func, entity_list<Es...>>
        { using type = entity_list<Func(Es)...>; };

    } // namespace tools_detail
    template <auto Func, typename EntityList>
    using bind_entity_list = tools_detail::bind_entity_list_impl<Func, EntityList>::type;
    template <typename T0, typename T1>
    using concat_entity_list = tools_detail::concat_entity_list_impl<T0, T1>::type;
    template <typename T, size_t N>
    using make_entity_sequence = tools_detail::make_entity_sequence_impl<T, std::make_integer_sequence<size_t, N>>::type;
    
    template <typename T, size_t N, tags::memory_location Tag = tags::memory_location::stack, typename Allocator = std::allocator<T>>
    struct chunk;
    
    template <typename T, size_t N, typename Allocator>
    struct chunk<T, N, tags::memory_location::heap, Allocator>
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
    struct chunk<T, N, tags::memory_location::stack>
    {
        // no private member for Aggregate initialization.
        T m_bin[N];
        constexpr T* ptr(size_t pos = 0) noexcept { return static_cast<T*>(m_bin) + pos; }
        constexpr const T* ptr(size_t pos = 0) const noexcept { return static_cast<const T*>(m_bin) + pos; }
        constexpr T& ref(size_t pos = 0) noexcept { return this->m_bin[pos]; }
        constexpr const T& ref(size_t pos = 0) const noexcept { return this->m_bin[pos]; }
        constexpr T& operator[](const size_t& pos) noexcept { return this->m_bin[pos]; }
        constexpr const T& operator[](const size_t& pos) const noexcept { return this->m_bin[pos]; }
    };
} // namespace sia
