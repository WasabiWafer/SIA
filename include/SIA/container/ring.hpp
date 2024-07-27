#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    template <typename T , size_t Size, typename Allocator = std::allocator<T>>
    struct ring
    {
    private:
        using allocator_traits_t = std::allocator_traits<Allocator>;
        compressed_pair<Allocator, T*> compair;

        constexpr void destruct_elem() noexcept {
            if (compair.second() != nullptr)
            {
                for(T* beg{compair.second()}; beg != (compair.second() + Size); ++beg)
                {
                    beg->~T();
                }
            }
        }
    public:
        constexpr ring() noexcept 
            : compair(compressed_pair_tag::zero, nullptr) {
            compair.second() = allocator_traits_t::allocate(compair.first(), Size);
        }
        
        constexpr ring(const ring& arg) noexcept
            : compair(compressed_pair_tag::zero, nullptr) {
            auto& target = compair.second();
            target = allocator_traits_t::allocate(compair.first(), Size);
            std::memcpy(target, arg.compair.second(), sizeof(T) * Size);
        }
        
        constexpr ring(ring&& arg) noexcept
            : compair(compressed_pair_tag::zero, arg.compair.second()) {
            arg.compair.second() = nullptr;
        }

        template <typename... Cs>
        constexpr ring(Cs&&... args) noexcept
            : compair(compressed_pair_tag::zero, nullptr) {
            compair.second() = allocator_traits_t::allocate(compair.first(), Size);
            auto target = compair.second();
            (allocator_traits_t::construct(compair.first(), target++, std::forward<Cs>(args)), ...);
        }

        constexpr ring& operator=(const ring& arg) noexcept
        {
            destruct_elem();
            std::memcpy(compair.second(), arg.compair.second(), sizeof(T) * Size);
            return *this;
        }
        constexpr ring& operator=(ring&& arg) noexcept
        {
            destruct_elem();
            auto& target = compair.second();
            if (target != nullptr) {
                allocator_traits_t::deallocate(compair.first(), target, Size);
            }
            target = arg.compair.second();
            arg.compair.second() = nullptr;
            return *this;
        }
        ~ring()
        {
            destruct_elem();
            allocator_traits_t::deallocate(compair.first(), compair.second(), Size);
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr T* begin(this auto&& self) noexcept { return self.compair.second(); }
        constexpr T* end(this auto&& self) noexcept { return self.begin() + Size; }
        constexpr T& operator[](this auto&& self, size_t pos) noexcept { return self.begin()[pos % self.m_cap]; }
        constexpr T* address(this auto&& self, size_t pos) noexcept { return self.begin()+(pos % self.m_cap); }
    };
} // namespace sia
