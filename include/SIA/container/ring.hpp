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
            for(T* beg{compair.second()}; beg != (compair.second() + Size); ++beg)
            {
                beg->~T();
            }
        }

    public:
        constexpr ring() noexcept 
            : compair(compressed_pair_tag::zero, allocator_traits_t::allocate(compair.first(), Size)) {
            for (auto& elem : *this) { allocator_traits_t::construct(compair.first(), &elem); }
        }

        template <typename... Cs> requires (sizeof...(Cs) <= Size)
        constexpr ring(Cs&&... args) noexcept
            : compair(compressed_pair_tag::zero, nullptr) {
            compair.second() = allocator_traits_t::allocate(compair.first(), Size);
            auto target = compair.second();
            (allocator_traits_t::construct(compair.first(), target++, std::forward<Cs>(args)), ...);
        }

        constexpr ring(const Allocator& arg) noexcept
            : compair(compressed_pair_tag::one, arg, nullptr) {
            compair.second() = allocator_traits_t::allocate(compair.first(), Size);
            for (auto& elem : *this) { allocator_traits_t::construct(compair.first(), &elem); }
        }

        template <typename... Cs> requires (sizeof...(Cs) <= Size)
        constexpr ring(const Allocator& arg, Cs&&... args) noexcept
            : compair(compressed_pair_tag::one, arg, nullptr) {
            compair.second() = allocator_traits_t::allocate(compair.first(), Size);
            auto target = compair.second();
            (allocator_traits_t::construct(compair.first(), target++, std::forward<Cs>(args)), ...);
        }

        constexpr ring(const ring& arg) noexcept
            : compair(compressed_pair_tag::one, arg.compair.first(), nullptr) {
            auto& target = compair.second();
            target = allocator_traits_t::allocate(compair.first(), Size);
            std::memcpy(target, arg.compair.second(), sizeof(T) * Size);
        }
        
        constexpr ring(ring&& arg) noexcept
            : compair(compressed_pair_tag::one, arg.compair.first(), arg.compair.second()) {
            arg.compair.second() = nullptr;
        }

        constexpr ring& operator=(const ring& arg) noexcept
        {
            auto& target = compair.second();
            if (target != nullptr) {
                destruct_elem();
                allocator_traits_t::deallocate(compair.first(), target, Size);
            }
            std::memcpy(target, arg.compair.second(), sizeof(T) * Size);
            return *this;
        }

        constexpr ring& operator=(ring&& arg) noexcept
        {
            auto& target = compair.second();
            if (target != nullptr) {
                destruct_elem();
                allocator_traits_t::deallocate(compair.first(), target, Size);
            }
            target = arg.compair.second();
            arg.compair.second() = nullptr;
            return *this;
        }

        ~ring()
        {
            if (compair.second() != nullptr)
            {
                destruct_elem();
                allocator_traits_t::deallocate(compair.first(), compair.second(), Size);
            }
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr T* begin(this auto&& self) noexcept { return self.compair.second(); }
        constexpr T* end(this auto&& self) noexcept { return self.begin() + Size; }
        constexpr T& operator[](this auto&& self, size_t pos) noexcept { return self.begin()[pos % Size]; }
        constexpr T* address(this auto&& self, size_t pos) noexcept { return self.begin() + (pos % Size); }
        template <typename... Cs> constexpr void emplace(size_t pos, Cs&&... args) noexcept { allocator_traits_t::construct(compair.first(), address(pos), std::forward<Cs>(args)...); }
        template <typename C> constexpr void push(size_t pos, C&& arg) noexcept { operator[](pos) = std::forward<C>(arg); }
        constexpr void destroy(size_t pos) noexcept { allocator_traits_t::destroy(compair.first(), address(pos)); }
    };
} // namespace sia
