#pragma once

#include <type_traits>

namespace sia
{
    namespace compressed_pair_tag
    {
        namespace tag_detail
        {
            enum class zero{ };
            enum class one{ };
        } // namespace tag_detail
        using zero_t = tag_detail::zero;
        using one_t = tag_detail::one;
        constexpr auto zero = tag_detail::zero{ };
        constexpr auto one = tag_detail::one{ };
    } // namespace composit_pait_tag
    
    template <typename T0, typename T1, bool = std::is_empty_v<T0> && !std::is_final_v<T0>>
    struct compressed_pair final : private T0
    {
    private:
        T1 sec;
    public:
        template <typename... Cs>
        explicit constexpr compressed_pair(compressed_pair_tag::zero_t, Cs&&... args) noexcept : T0(), sec(std::forward<Cs>(args)...) { }
        template <typename C, typename... Cs>
        explicit constexpr compressed_pair(compressed_pair_tag::one_t, C&& arg, Cs&&... args) noexcept : T0(std::forward<C>(arg)), sec(std::forward<Cs>(args)...) { }
        constexpr T0& first(this auto&& self) noexcept {
            return self;
        }
        constexpr T1& second(this auto&& self) noexcept {
            return self.sec;
        }
    };

    template <typename T0, typename T1>
    struct compressed_pair<T0, T1, false> final
    {
    private:
        T0 fir;
        T1 sec;
    public:
        template <typename... Cs>
        explicit constexpr compressed_pair(compressed_pair_tag::zero_t, Cs&&... args) noexcept : fir(), sec(std::forward<Cs>(args)...) { }
        template <typename C, typename... Cs>
        explicit constexpr compressed_pair(compressed_pair_tag::one_t, C&& arg, Cs&&... args) noexcept : fir(std::forward<C>(arg)), sec(std::forward<Cs>(args)...) { }
        constexpr T0& first(this auto&& self) noexcept {
            return self.fir;
        }
        constexpr T1& second(this auto&& self) noexcept {
            return self.sec;
        }
    };
} // namespace sia