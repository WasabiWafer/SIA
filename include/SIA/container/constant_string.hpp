#pragma once

#include <type_traits>
#include <utility>

#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace constant_string_detail
    {
        template <typename T> concept Character_req = is_same_any_v<std::remove_cvref_t<T>, char, wchar_t, char16_t, char32_t>;
        template <typename T> concept Array_req = std::is_array_v<std::remove_cvref_t<T>>;
        template <typename T> concept Constant_string_req = Array_req<T> && Character_req<std::remove_extent_t<std::remove_cvref_t<T>>>;

        template <typename T, size_t N>
        struct constant_string_data
        {
            T m_data[N];

            template <typename Ty, size_t... Idxs>
            constexpr constant_string_data(Ty&& arr, std::index_sequence<Idxs...>) noexcept : m_data(arr[Idxs]...) { }
            template <typename Ty, size_t... Idxs>
            constexpr constant_string_data(const constant_string_data& arg, std::index_sequence<Idxs...>) noexcept : m_data(arg[Idxs]...) { }
            constexpr constant_string_data& operator=(const constant_string_data& arg) noexcept
            {
                for (size_t idx = 0; idx < N; ++idx)
                {
                    m_data[idx] = arg.m_data[idx];
                }
                return *this;
            }
            constexpr T operator[](this auto&& self, size_t pos) noexcept { return self.m_data[pos]; }
        };
    } // namespace constant_string_detail
    
    template <constant_string_detail::Character_req T, size_t N>
    struct constant_string
    {
        constant_string_detail::constant_string_data<T, N> m_wrap;

        constexpr constant_string(T (&arg)[N]) noexcept : m_wrap(arg, std::make_index_sequence<N>()) { }
        constexpr constant_string(const constant_string& arg) noexcept : m_wrap(arg.m_wrap, std::make_index_sequence<N>()) { }
        constexpr constant_string& operator=(const constant_string& arg) noexcept
        {
            m_wrap.m_data = arg.m_wrap.m_data;
            return *this;
        }

        constexpr T* begin(this auto&& self) noexcept { return self.m_wrap.m_data; }
        constexpr T* end(this auto&& self) noexcept { return self.m_wrap.m_data + N; }
    };

    template <constant_string_detail::Constant_string_req T>
    constant_string(T&&) -> constant_string<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
} // namespace sia
