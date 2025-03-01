#pragma once

#include <type_traits>

#include "SIA/internals/define.hpp"

namespace sia
{    
    template <typename T0, typename T1, bool = std::is_empty_v<T0> && !std::is_final_v<T0>>
    struct compressed_pair final : private T0
    {
    private:
        T1 m_sec;
    public:
        template <typename... Cs>
        constexpr compressed_pair(splits::zero_t, Cs&&... args) noexcept(noexcept(T0()) && noexcept(T1(Cs(args)...)))
            : T0(), m_sec(std::forward<Cs>(args)...)
        { }
        template <typename C, typename... Cs>
        constexpr compressed_pair(splits::one_t, C&& arg, Cs&&... args) noexcept(noexcept(T0(C(arg))) && noexcept(T1(Cs(args)...)))
            : T0(std::forward<C>(arg)), m_sec(std::forward<Cs>(args)...)
        { }
        constexpr T0& first(this auto&& self) noexcept { return self; }
        constexpr T1& second(this auto&& self) noexcept { return self.m_sec; }
    };

    template <typename T0, typename T1>
    struct compressed_pair<T0, T1, false> final
    {
    private:
        T0 m_fir;
        T1 m_sec;
    public:
        template <typename... Cs>
        constexpr compressed_pair(splits::zero_t, Cs&&... args) noexcept(noexcept(T0()) && noexcept(T1(Cs(args)...)))
            : m_fir(), m_sec(std::forward<Cs>(args)...)
        { }
        template <typename C, typename... Cs>
        constexpr compressed_pair(splits::one_t, C&& arg, Cs&&... args) noexcept(noexcept(T0(C(arg))) && noexcept(T1(Cs(args)...)))
            : m_fir(std::forward<C>(arg)), m_sec(std::forward<Cs>(args)...)
        { }
        constexpr T0& first(this auto&& self) noexcept { return self.m_fir; }
        constexpr T1& second(this auto&& self) noexcept { return self.m_sec; }
    };
} // namespace sia