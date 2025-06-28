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
        template <typename T> concept CharType = is_same_any_v<std::remove_cvref_t<T>, signed char, unsigned char, char, wchar_t, char16_t, char32_t>;
        template <typename T> concept ArrType = std::is_array_v<std::remove_cvref_t<T>>;
        template <typename T> concept CStrReq = ArrType<T> && CharType<std::remove_extent_t<std::remove_cvref_t<T>>>;
    } // namespace constant_string_detail

    template <constant_string_detail::CharType T, size_t N, typename SeqType = std::make_index_sequence<N>>
    struct constant_string;

    template <constant_string_detail::CharType T, size_t N, size_t... Seqs>
    struct constant_string<T, N, std::index_sequence<Seqs...>> : public chunk<T, N>
    {
        private:
            using base_t = chunk<T, N>;
        public:
            constexpr constant_string() noexcept = default;

            constexpr constant_string(const constant_string&) noexcept = default;
            constexpr constant_string& operator=(const constant_string&) noexcept = default;

            constexpr constant_string(constant_string&&) noexcept = default;
            constexpr constant_string& operator=(constant_string&&) noexcept = default;

            constexpr constant_string(const T (&arg)[N]) noexcept : base_t(arg[Seqs]...) { }
            template <constant_string_detail::CharType Ty, size_t Size>
                requires (Size <= N)
            constexpr constant_string& operator=(const Ty(&arg)[Size]) noexcept
            {
                size_t idx{ };
                // if (Size < N)
                // {
                    for (; idx < Size; ++idx)
                    { this->ref(idx) = arg[idx]; }
                    for (; idx < N; ++idx)
                    { this->ref(idx) = '\0'; }
                // }
                // else
                // {
                //     for (; idx < N - 1; ++idx)
                //     { this->ref(idx) = arg[idx]; }
                //     this->ref(idx) = '\0';
                // }
                return *this;
            }

            constexpr size_t length(this auto&& self) noexcept { return N; }
            constexpr       T* begin()  noexcept        { return this->ptr(); }
            constexpr const T* begin()  const noexcept  { return this->ptr(); }
            constexpr       T* end()    noexcept        { return this->begin() + this->length(); }
            constexpr const T* end()    const noexcept  { return this->begin() + this->length(); }
            constexpr std::string_view to_string_view(this auto&& self) noexcept { return std::string_view(self.begin(), self.length()); }
            constexpr std::string to_string(this auto&& self) noexcept { return std::string(self.begin(), self.length()); }
            
            constexpr bool operator==(this auto&& self, const constant_string& arg) noexcept
            {
                for
                    (
                        T* l_begin{self.begin()}, * r_begin{arg.begin()};
                        l_begin < self.end();
                        ++l_begin, ++r_begin
                    )
                {
                    if (*l_begin != *r_begin)
                    { return false; }
                }
                return true;
            }

            template <typename Ty, size_t Size>
                requires (Size <= N)
            constexpr bool operator==(this auto&& self, const Ty (&arg)[Size]) noexcept
            {
                for (T* l_begin{self.begin()}; auto& elem : arg)
                {
                    if (*l_begin != elem)
                    { return false; }
                }
                return true;
            }
            
            template <typename Ty>
            constexpr bool operator!=(this auto&& self, const Ty& arg) noexcept
            { return !self.operator==(arg); }
    };

    template <constant_string_detail::CStrReq T>
    constant_string(T&&) -> constant_string<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
} // namespace sia
