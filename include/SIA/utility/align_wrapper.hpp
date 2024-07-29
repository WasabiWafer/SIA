#pragma once

#include <new>

#include "SIA/internals/types.hpp"

namespace sia
{
    namespace align_wrapper_detail
    {
        template <size_t Size>  struct byte_padding     { unsigned_interger_t<1> pad[Size]; };
        template <>             struct byte_padding<0>  { };
    } // namespace align_wrapper_detail
    
    //align_wrapper
    template <typename T, size_t Align> requires (alignof(T) <= Align)
    struct align_wrapper : private align_wrapper_detail::byte_padding<Align - alignof(T)>
    {
    private:
        using padding_t = align_wrapper_detail::byte_padding<Align - alignof(T)>;
        T elem;

        align_wrapper(const align_wrapper&) = delete;
        align_wrapper& operator=(const align_wrapper&) = delete;
    public:
        constexpr align_wrapper() noexcept : padding_t(), elem() { }
        template <typename... Cs>
        constexpr align_wrapper(Cs&&... args) noexcept : padding_t(), elem(std::forward<Cs>(args)...) { }
        constexpr T& self(this auto&& self_) noexcept { return self_.elem; }
        constexpr T* operator->(this auto&& self) noexcept { return &(self.elem); }
    };

    template <typename T> using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename T> using true_share  = align_wrapper<T, std::hardware_constructive_interference_size>;
} // namespace sia
