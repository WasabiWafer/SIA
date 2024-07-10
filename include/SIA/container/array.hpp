#pragma once

#include <type_traits>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace array_detail
    {
        template <typename T>
        concept ArrType = std::is_array_v<std::remove_reference_t<T>>;
        template <typename T0, typename T1>
        concept ArrTypeRes = std::is_array_v<std::remove_reference_t<T1>> && std::is_same_v<std::remove_reference_t<T0>, std::remove_reference_t<T1>>;
        template <typename T>
        concept ElemType = !(std::is_array_v<std::remove_reference_t<T>>);
        template <typename T0, typename T1>
        concept ElemTypeRes = std::is_same_v<std::remove_reference_t<T0>, std::remove_reference_t<T1>> && !(std::is_array_v<std::remove_reference_t<T0>> || std::is_array_v<std::remove_reference_t<T1>>);
    } // namespace array_detail
    

    template <typename T, size_t N>
    struct array
    {
        T elem[N];
        constexpr T* begin  (this auto&& self)  noexcept    { return self.elem; }
        constexpr T* end    (this auto&& self)  noexcept    { return self.elem + N; }
        constexpr T& front  (this auto&& self)  noexcept    { return self.elem[0]; }
        constexpr T& back   (this auto&& self)  noexcept    { return self.elem[N-1]; }
        constexpr T& at     (this auto&& self, const size_t pos)    noexcept { return self.elem[pos]; }
        constexpr T& operator[](this auto&& self, const size_t pos) noexcept { return self.elem[pos]; }
        constexpr size_t size(this auto&& self) noexcept    { return N; }
    };
    template <typename T, size_t N0, size_t N1>
    struct array<array<T, N0>, N1>
    {
        array<T, N0> elem[N1];
        constexpr auto* begin  (this auto&& self)  noexcept    { return self.elem[0].begin(); }
        constexpr auto* end    (this auto&& self)  noexcept    { return self.elem[N1-1].end(); }
        constexpr auto& front  (this auto&& self)  noexcept    { return self.elem[0].front(); }
        constexpr auto& back   (this auto&& self)  noexcept    { return self.elem[N1-1].back(); }
        constexpr auto& at     (this auto&& self, const size_t pos)     noexcept { return self.elem->at(pos); }
        constexpr auto& operator[](this auto&& self, const size_t pos)  noexcept { return self.elem[pos]; }
        constexpr size_t size(this auto&& self) noexcept
        {
            size_t ret{ };
            for (auto& elems : self.elem) { ret += elems.size(); }
            return ret;
        }
    };

    template <array_detail::ArrType T>
    array(T&& arg) -> array<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
    template <array_detail::ArrType T, array_detail::ArrTypeRes<T>... Ts>
    array(T&& arg, Ts&&... args) -> array<array<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>, sizeof...(Ts) + 1>;
    template <array_detail::ElemType T, array_detail::ElemTypeRes<T>... Ts>
    array(T&&, Ts&&...) -> array<std::remove_reference_t<T>, sizeof...(Ts) + 1>;
} // namespace sia
