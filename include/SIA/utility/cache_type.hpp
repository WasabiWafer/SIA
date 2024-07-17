#pragma once

#include <new>

#include "SIA/internals/types.hpp"

namespace sia
{
    template <typename T>
    struct false_share
    {
    private:
        alignas(std::hardware_destructive_interference_size) T elem;
        unsigned_interger_t<1> pad[std::hardware_destructive_interference_size - sizeof(T)];

        false_share(const false_share&) = delete;
        false_share& operator=(const false_share&) = delete;
    public:
        constexpr false_share() noexcept : elem(), pad() { }
        template <typename... Ts>
        constexpr false_share(Ts&&... args) noexcept : elem(std::forward<Ts>(args)...), pad() { }

        constexpr T& self() noexcept { return elem; }
        constexpr T* operator->() noexcept { return &elem; }
    };
    template <typename T>
    false_share(T) -> false_share<T>;

    template <typename T>
    struct true_share
    {
    private:
        alignas(std::hardware_constructive_interference_size) T elem;
        unsigned_interger_t<1> pad[std::hardware_constructive_interference_size - sizeof(T)];

        true_share(const true_share&) = delete;
        true_share& operator=(const true_share&) = delete;
    public:
        constexpr true_share() noexcept : elem(), pad() { }
        template <typename... Ts>
        constexpr true_share(Ts&&... args) noexcept : elem(std::forward<Ts>(args)...), pad() { }

        constexpr T& self() noexcept { return elem; }
        constexpr T* operator->() noexcept { return &elem; }
    };
    template <typename T>
    true_share(T) -> true_share<T>;
} // namespace sia
