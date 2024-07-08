#pragma once

#include <bit>
#include <type_traits>

namespace sia
{
    namespace array_detail
    {
        template <typename T>
        concept ArrInitReq = std::is_array_v<std::remove_reference_t<T>>;
        template <typename T0, typename T1>
        concept ElemInitReq = std::is_same_v<std::remove_reference_t<T0>, std::remove_reference_t<T1>> && !(std::is_array_v<std::remove_reference_t<T0>> || std::is_array_v<std::remove_reference_t<T1>>);
    } // namespace array_detail
    

    template <typename T, size_t N>
    struct array
    {
        T arr[N];

        constexpr T* begin  (this auto&& self)  noexcept    { return self.arr; }
        constexpr T* end    (this auto&& self)  noexcept    { return self.arr + N; }
        constexpr T& front  (this auto&& self)  noexcept    { return self.arr[0]; }
        constexpr T& back   (this auto&& self)  noexcept    { return self.arr[N-1]; }
        constexpr size_t size(this auto&& self) noexcept    { return N; }
        constexpr T& operator[](this auto&& self, const size_t pos) noexcept { return self.arr[pos]; }
    };

    template <array_detail::ArrInitReq T>
    array(T&& arg) -> array<std::remove_extent_t<std::remove_reference_t<T>>, std::extent_v<std::remove_reference_t<T>>>;
    template <typename T, array_detail::ElemInitReq<T>... Ts> requires (!(std::is_array_v<std::remove_reference_t<T>>))
    array(T&&, Ts&&...) -> array<std::remove_reference_t<T>, sizeof...(Ts) + 1>;
} // namespace sia
