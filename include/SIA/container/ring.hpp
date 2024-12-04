#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    template <typename T , size_t Size, typename Allocator = std::allocator<T>>
    struct ring {
    private:
        using allocator_traits_t = std::allocator_traits<Allocator>;
        compressed_pair<Allocator, T*> m_compair;

    public:
        constexpr ring(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr)
        {
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), Size);
            for (auto& elem : *this) { allocator_traits_t::construct(m_compair.first(), &elem); }
        }

        constexpr ring(std::initializer_list<T> arg, const Allocator& alloc = Allocator()) noexcept
            : m_compair(compressed_pair_tag::one, alloc, nullptr)
        {
            static_assert(arg.size() <= Size);
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), Size);
            auto target = m_compair.second();
            for (auto& elem : arg)
            {
                allocator_traits_t::construct(m_compair.first(), target++, elem);
            }
        }

        // constexpr ring(const ring& arg, const Allocator& alloc = Allocator()) noexcept
        //     : m_compair(compressed_pair_tag::one, alloc, nullptr)
        // {
        //     auto& target = m_compair.second();
        //     target = allocator_traits_t::allocate(m_compair.first(), Size);
        //     std::memcpy(target, arg.m_compair.second(), sizeof(T) * Size);
        // }
        
        // constexpr ring(ring&& arg, const Allocator& alloc = Allocator()) noexcept
        //     : m_compair(compressed_pair_tag::one, alloc, arg.m_compair.second())
        // { arg.m_compair.second() = nullptr; }

        // constexpr ring& operator=(const ring& arg) noexcept
        // {
        //     auto& target = m_compair.second();
        //     if (target != nullptr) {
        //         allocator_traits_t::deallocate(m_compair.first(), target, Size);
        //     }
        //     std::memcpy(target, arg.m_compair.second(), sizeof(T) * Size);
        //     return *this;
        // }

        // constexpr ring& operator=(ring&& arg) noexcept
        // {
        //     auto& target = m_compair.second();
        //     if (target != nullptr) {
        //         allocator_traits_t::deallocate(m_compair.first(), target, Size);
        //     }
        //     target = arg.m_compair.second();
        //     arg.m_compair.second() = nullptr;
        //     return *this;
        // }

        ~ring()
        {
            if (m_compair.second() != nullptr) {
                allocator_traits_t::deallocate(m_compair.first(), m_compair.second(), Size);
            }
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr Allocator& get_alloc() noexcept { return m_compair.first(); }
        constexpr T* begin(this auto&& self) noexcept { return self.m_compair.second(); }
        constexpr T* end(this auto&& self) noexcept { return self.begin() + Size; }
        constexpr T& operator[](this auto&& self, size_t pos) noexcept { return self.begin()[pos % Size]; }
        constexpr T* address(this auto&& self, size_t pos) noexcept { return self.begin() + (pos % Size); }
        template <typename... Cs> constexpr void emplace(size_t pos, Cs&&... args) noexcept { allocator_traits_t::construct(m_compair.first(), address(pos), std::forward<Cs>(args)...); }
        template <typename C> constexpr void push(size_t pos, C&& arg) noexcept { operator[](pos) = std::forward<C>(arg); }
    };
} // namespace sia
