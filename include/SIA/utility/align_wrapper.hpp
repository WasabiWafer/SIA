#pragma once

#include <new>

#include "SIA/internals/types.hpp"

namespace sia
{
    //align_wrapper
    template <typename T, size_t Align>
    struct align_wrapper
    {
    private:
        alignas(Align) T elem;

        align_wrapper(const align_wrapper&) = delete;
        align_wrapper& operator=(const align_wrapper&) = delete;
    public:
        constexpr align_wrapper() noexcept : elem() { }
        template <typename... Ts>
        constexpr align_wrapper(Ts&&... args) noexcept : elem(std::forward<Ts>(args)...) { }

        constexpr T& self(this auto&& self_) noexcept { return self_.elem; }
        constexpr T* operator->(this auto&& self) noexcept { return &(self.elem); }
    };
    template <typename T>
    using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename T>
    using true_share = align_wrapper<T, std::hardware_constructive_interference_size>;
} // namespace sia
