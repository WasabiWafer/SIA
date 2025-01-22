#pragma once

#include <type_traits>
#include <utility>

#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace constant_string_detail
    {
        template <typename T> concept Character_req = is_same_any_v<std::remove_cvref_t<T>, signed char, unsigned char, char, wchar_t, char16_t, char32_t>;
        template <typename T> concept Array_req = std::is_array_v<std::remove_cvref_t<T>>;
        template <typename T> concept Constant_string_req = Array_req<T> && Character_req<std::remove_extent_t<std::remove_cvref_t<T>>>;

        template <typename T, size_t N>
        struct constant_string_impl : public chunk<T, N>
        {
        private:
            using base_t = chunk<T, N>;
        public:
            constexpr constant_string_impl() noexcept : base_t() { }
            template <size_t... Idxs>
            constexpr constant_string_impl(const constant_string_impl& arg, std::index_sequence<Idxs...>) noexcept : base_t{arg.m_bin[Idxs]...} { }
            template <size_t... Idxs>
            constexpr constant_string_impl(const T (&arg)[N], std::index_sequence<Idxs...>) noexcept : base_t{arg[Idxs]...} { }
        };
    } // namespace constant_string_detail
    
    template <constant_string_detail::Character_req T, size_t N>
    struct constant_string : public constant_string_detail::constant_string_impl<T, N>
    {
    private:
        using base_t = constant_string_detail::constant_string_impl<T, N>;
    public:
        constexpr constant_string() noexcept : base_t() { }
        constexpr constant_string(const constant_string& arg) noexcept : base_t(static_cast<base_t>(arg), std::make_index_sequence<N>()) { }
        constexpr constant_string(const T (&arg)[N]) noexcept : base_t(arg, std::make_index_sequence<N>()) { }
        constexpr constant_string& operator=(this auto&& self, const constant_string& arg) noexcept
        {
            for (T* x = self.m_bin, *y = arg.m_bin; x < (self.m_bin + N); ++x, ++y) { *x = *y; }
            return self;
        }
        constexpr constant_string& operator=(this auto&& self, const T (&arg)[N]) noexcept
        {
            for (struct {T* x; const T* y;} obj = {self.m_bin, arg}; obj.x < (self.m_bin + N); ++obj.x, ++obj.y) { *obj.x = *obj.y; }
            return self;
        }

        constexpr       T* begin()  noexcept        { return this->m_bin; }
        constexpr const T* begin()  const noexcept  { return this->m_bin; }
        constexpr       T* end()    noexcept        { return this->m_bin + N; }
        constexpr const T* end()    const noexcept  { return this->m_bin + N; }
        constexpr std::string_view to_string_view(this auto&& self) noexcept { return std::string_view(self.begin(), N); }
        constexpr std::string to_string(this auto&& self) noexcept { return std::string(self.begin(), N); }
    };

    template <constant_string_detail::Constant_string_req T>
    constant_string(T&&) -> constant_string<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
} // namespace sia
