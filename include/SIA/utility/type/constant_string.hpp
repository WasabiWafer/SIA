#pragma once

#include <type_traits>
#include <utility>
#include <string>
#include <string_view>

#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace constant_string_detail
    {
        template <typename T> concept CharReq = is_same_any_v<std::remove_cvref_t<T>, signed char, unsigned char, char, wchar_t, char16_t, char32_t>;
        template <typename T> concept ArrReq = std::is_array_v<std::remove_cvref_t<T>>;
        template <typename T> concept CStrReq = ArrReq<T> && CharReq<std::remove_extent_t<std::remove_cvref_t<T>>>;

        template <typename T, size_t N>
        struct constant_string_impl : public chunk<T, N>
        {
        private:
            using base_t = chunk<T, N>;
        public:
            constexpr constant_string_impl() noexcept = default;
            template <size_t... Idxs>
            constexpr constant_string_impl(const T (&arg)[N], std::index_sequence<Idxs...>) noexcept : base_t{arg[Idxs]...} { }
        };
    } // namespace constant_string_detail
    
    template <constant_string_detail::CharReq T, size_t N>
    struct constant_string : public constant_string_detail::constant_string_impl<T, N>
    {
    private:
        using base_t = constant_string_detail::constant_string_impl<T, N>;
    public:
        constexpr constant_string() noexcept = default;
        constexpr constant_string(const T (&arg)[N]) noexcept : base_t(arg, std::make_index_sequence<N>()) { }
        constexpr size_t length(this auto&& self) noexcept { return N; }
        constexpr       T* begin()  noexcept        { return this->ptr(); }
        constexpr const T* begin()  const noexcept  { return this->ptr(); }
        constexpr       T* end()    noexcept        { return this->begin() + this->length(); }
        constexpr const T* end()    const noexcept  { return this->begin() + this->length(); }
        constexpr std::string_view to_string_view(this auto&& self) noexcept { return std::string_view(self.begin(), self.length()); }
        constexpr std::string to_string(this auto&& self) noexcept { return std::string(self.begin(), self.length()); }
    };

    template <constant_string_detail::CStrReq T>
    constant_string(T&&) -> constant_string<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
} // namespace sia
