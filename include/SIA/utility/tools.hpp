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
    
    // namespace tools_detail
    // {
    //     template <auto Func, typename Arg, typename... Args>
    //     struct is_nothrow_raw_function : std::bool_constant<requires() { {Func(Arg{ }, Args{ }...)} noexcept; }> { };
    //     template <auto Func, typename... Args>
    //     constexpr const bool is_nothrow_function_v = is_nothrow_function<Func, Args...>::value;
    
    //     template <auto Func, typename Class, typename... Args>
    //     struct is_nothrow_member_function : std::bool_constant<requires(Class c) { {(c.*Func)(Args{ }...)} noexcept; }> { };
    //     template <typename Class, auto Func, typename... Args>
    //     constexpr const bool is_nothrow_member_function_v = is_nothrow_member_function<Class, Func, Args...>::value;
    // } // namespace tools_detail
    
    // template <auto Func, typename... Args>
    // struct is_nothrow_function;

    // template <auto Func, typename... Args>
    //     requires (std::is_pointer_v<decltype(Func)> && std::is_function_v<std::remove_pointer_t<decltype(Func)>>)
    // struct is_nothrow_function<Func, Args...> : std::bool_constant<requires() { {Func(Args{ }...)} noexcept; }> { };

    // template <auto Func, typename Class, typename... Args>
    //     requires (std::is_member_function_pointer_v<decltype(Func)>)
    // struct is_nothrow_function<Func, Class, Args...> : std::bool_constant<requires(Class c) { {(c.*Func)(Args{ }...)} noexcept; }> { };
    
    // template <auto Func, typename... Args>
    // constexpr const bool is_nothrow_function_v = is_nothrow_function<Func, Args...>::value;

    template <typename FuncType, typename... Args>
    struct is_nothrow_function;

    template <typename FpType, typename... Args>
        requires (std::is_pointer_v<FpType> && std::is_function_v<std::remove_pointer_t<FpType>>)
    struct is_nothrow_function<FpType, Args...> : std::bool_constant<requires(FpType fp) { { fp(Args{ }...) } noexcept; }> { };

    template <typename FpType, typename ClassPtr, typename... Args>
        requires (std::is_member_function_pointer_v<FpType>)
    struct is_nothrow_function<FpType, ClassPtr, Args...> : std::bool_constant<requires(FpType fp, ClassPtr c) { {(c->*fp)(Args{ }...)} noexcept; }> { };
    
    template <typename FpType, typename... Args>
    constexpr const bool is_nothrow_function_v = is_nothrow_function<FpType, Args...>::value;

    template <auto E>
    struct entity
    {
        using type = decltype(E);
        static constexpr const auto value = E;
        constexpr const auto& operator()(this auto&& self) noexcept { return value; }
    };

    template <typename T>
    using type = T;
    template <auto E>
    using to_type = decltype(E);
    template <typename T, auto... Es>
    using to_entity = entity<T(Es...)>;

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
        T m_chunk[N];
        constexpr T* ptr(size_t pos = 0) noexcept { return static_cast<T*>(m_chunk) + pos; }
        constexpr const T* ptr(size_t pos = 0) const noexcept { return static_cast<const T*>(m_chunk) + pos; }
        constexpr T& ref(size_t pos = 0) noexcept { return this->m_chunk[pos]; }
        constexpr const T& ref(size_t pos = 0) const noexcept { return this->m_chunk[pos]; }
        constexpr T& operator[](const size_t& pos) noexcept { return this->m_chunk[pos]; }
        constexpr const T& operator[](const size_t& pos) const noexcept { return this->m_chunk[pos]; }
    };
} // namespace sia
