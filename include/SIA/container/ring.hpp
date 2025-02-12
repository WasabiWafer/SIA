#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace ring_detail
    {
        template <typename T>
        struct ring_composition
        {
            T* m_data;
            size_t m_begin;
            size_t m_end;
        };
    } // namespace sia
    
    template <typename T, size_t Size, typename Allocator = std::allocator<T>>
    struct ring
    {
    private:
        using composition_t = ring_detail::ring_composition<T>;
        using allocator_traits_t = std::allocator_traits<Allocator>;

        compressed_pair<Allocator, composition_t> m_compair;
    public:
        constexpr ring(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr, 0, 0)
        {
            auto& allocator = this->m_compair.first();
            auto& comp = this->m_compair.second();
            comp.m_data = allocator_traits_t::allocate(allocator, Size);
        }

        constexpr ring(const std::initializer_list<T>& arg, const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr, 0, 0)
        {
            assertm(arg.size() <= Size, "Error : initialize with oversize.");
            auto& allocator = this->m_compair.first();
            auto& comp = this->m_compair.second();
            comp.m_data = allocator_traits_t::allocate(allocator, Size);
            for (auto& elem : arg)
            {
                allocator_traits_t::construct(allocator, comp.m_data + comp.m_end, elem);
                ++comp.m_end;
            }
        }

        ~ring()
        {
            auto& allocator = this->m_compair.first();
            auto& comp = this->m_compair.second();
            if (m_compair.second().m_data != nullptr)
            { allocator_traits_t::deallocate(allocator, comp.m_data, Size); }
        }

        [[nodiscard]]
        constexpr T& operator[](this auto&& self, size_t idx) noexcept
        {
            assertm(idx < self.size(), "Error : access violation");
            auto& comp = self.m_compair.second();
            return *(comp.m_data + ((comp.m_begin + idx) % self.capacity()));
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr size_t size(this auto&& self) noexcept
        {
            auto& comp = self.m_compair.second();
            return comp.m_end - comp.m_begin;
        }
        constexpr bool is_full(this auto&& self)    noexcept { return self.size() == self.capacity(); }
        constexpr bool is_empty(this auto&& self)   noexcept { return self.size() == 0; }

        [[nodiscard]]
        constexpr T& at(size_t idx) noexcept
        {
            assertm(idx < this->size(), "Error : access violation");
            auto& comp = this->m_compair.second();
            return *(comp.m_data + ((comp.m_begin + idx) % this->capacity()));
        }
        
        template <typename... Tys>
        constexpr bool try_emplace_back(Tys&&... args)
        {
            if (this->is_full()) { return false; }
            else
            {
                auto& alloc = this->m_compair.first();
                auto& comp = this->m_compair.second();
                allocator_traits_t::construct(alloc, comp.m_data + (comp.m_end % this->capacity()), std::forward<Tys>(args)...);
                ++comp.m_end;
                return true;
            }
        }
        constexpr bool try_push_back(const T& arg)  { return this->try_emplace_back(arg); }
        constexpr bool try_push_back(T&& arg)       { return this->try_emplace_back(std::move(arg)); }
        constexpr void pop_front(this auto&& self)
        {
            auto& alloc = self.m_compair.first();
            auto& comp = self.m_compair.second();
            if (!self.is_empty())
            {
                allocator_traits_t::destroy(alloc, comp.m_data + (comp.m_begin % self.capacity()));
                ++comp.m_begin;
            }
        }

        // [[nodiscard]]
        // constexpr T& back() noexcept
        // {
        //     assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
        //     auto& comp = this->m_compair.second();
        //     return *(comp.m_data + ((comp.m_end - 1) % this->capacity()));
        // }

        // [[nodiscard]]
        // constexpr const T& back() const noexcept
        // {
        //     assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
        //     auto& comp = this->m_compair.second();
        //     return *(comp.m_data + ((comp.m_end - 1) % this->capacity()));
        // }
        
        [[nodiscard]]
        constexpr T& front() noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            auto& comp = this->m_compair.second();
            return *(comp.m_data + (comp.m_begin) % this->capacity());
        }

        [[nodiscard]]
        constexpr const T& front() const noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            auto& comp = this->m_compair.second();
            return *(comp.m_data + (comp.m_begin) % this->capacity());
        }
    };
} // namespace sia
