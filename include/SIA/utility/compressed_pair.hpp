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
        template <typename... Tys>
        constexpr compressed_pair(splits::zero_t, Tys&&... args) noexcept(std::is_nothrow_default_constructible_v<T0> && std::is_nothrow_constructible_v<T1, Tys...>)
            : T0(), m_sec(std::forward<Tys>(args)...)
        { }
        template <typename Ty, typename... Tys>
        constexpr compressed_pair(splits::one_t, Ty&& arg, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T0, Ty> && std::is_nothrow_constructible_v<T1, Tys...>)
            : T0(std::forward<Ty>(arg)), m_sec(std::forward<Tys>(args)...)
        { }
        constexpr T0& first() noexcept { return *static_cast<T0*>(this); }
        constexpr const T0& first() const noexcept { return *static_cast<const T0*>(this); }
        constexpr T1& second() noexcept { return this->m_sec; }
        constexpr const T1& second() const noexcept { return this->m_sec; }
    };

    template <typename T0, typename T1>
    struct compressed_pair<T0, T1, false> final
    {
    private:
        T0 m_fir;
        T1 m_sec;
    public:
        template <typename... Tys>
        constexpr compressed_pair(splits::zero_t, Tys&&... args) noexcept(std::is_nothrow_default_constructible_v<T0> && std::is_nothrow_constructible_v<T1, Tys...>)
            : m_fir(), m_sec(std::forward<Tys>(args)...)
        { }
        template <typename Ty, typename... Tys>
        constexpr compressed_pair(splits::one_t, Ty&& arg, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T0, Ty> && std::is_nothrow_constructible_v<T1, Tys...>)
            : m_fir(std::forward<Ty>(arg)), m_sec(std::forward<Tys>(args)...)
        { }
        constexpr T0& first() noexcept { return this->m_fir; }
        constexpr const T0& first() const noexcept { return this->m_fir; }
        constexpr T1& second() noexcept { return this->m_sec; }
        constexpr const T1& second() const noexcept { return this->m_sec; }
    };
} // namespace sia