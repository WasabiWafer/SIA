#pragma once

#include <new>

#include "SIA/internals/types.hpp"

namespace sia
{
    namespace align_wrapper_detail
    {
        template <typename T, size_t Align>
        struct share
        {
        private:
            alignas(Align) T elem;

            share(const share&) = delete;
            share& operator=(const share&) = delete;
        public:
            constexpr share() noexcept : elem() { }
            template <typename... Ts>
            constexpr share(Ts&&... args) noexcept : elem(std::forward<Ts>(args)...) { }

            constexpr T& self(this auto&& self_) noexcept { return self_.elem; }
            constexpr T* operator->(this auto&& self) noexcept { return &(self.elem); }
        };
    } // namespace align_wrapper_detail
    template <typename T, size_t Align>
    using align_wrapper = align_wrapper_detail::share<T, Align>;
    template <typename T>
    using false_share = align_wrapper_detail::share<T, std::hardware_destructive_interference_size>;
    template <typename T>
    using true_share = align_wrapper_detail::share<T, std::hardware_constructive_interference_size>;
} // namespace sia
