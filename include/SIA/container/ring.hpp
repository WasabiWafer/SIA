#pragma once

#include <memory>
#include <limits>

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
        using allocator_t = Allocator;
        using allocator_traits_t = std::allocator_traits<Allocator>;

        compressed_pair<Allocator, composition_t> m_compair;

        constexpr allocator_t& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr composition_t& get_composition(this auto&& self) noexcept { return self.m_compair.second(); }
        constexpr T* address(this auto&& self, size_t idx) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_data + ((comp.m_begin + idx) % self.capacity());
        }
        constexpr T* raw_address(this auto&& self, size_t idx) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_data + (idx % self.capacity());
        }
        constexpr T* begin(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return self.raw_address(comp.m_begin);
        }
        constexpr T* end(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return self.raw_address(comp.m_end);
        }
        constexpr void dec_pos(this auto&& self, size_t& pos) noexcept
        {
            composition_t& comp = self.get_composition();
            if (pos == 0)
            { pos = std::numeric_limits<size_t>::max() - (std::numeric_limits<size_t>::max() % self.capacity()) - 1; }
            else
            { --pos; }
        }
        constexpr void inc_pos(this auto&& self, size_t& pos) noexcept
        {
            composition_t& comp = self.get_composition();
            if (pos == std::numeric_limits<size_t>::max())
            { pos = std::numeric_limits<size_t>::max() % self.capacity() + 1; }
            else
            { ++pos; }
        }
    public:
        constexpr ring(const allocator_t& alloc = allocator_t()) noexcept(noexcept(allocator_t(alloc)) && noexcept(allocator_traits_t::allocate(this->get_allocator(), Size)))
            : m_compair(splits::one_v, alloc, nullptr, 0, 0)
        {
            composition_t& comp = this->get_composition();
            comp.m_data = allocator_traits_t::allocate(this->get_allocator(), Size);
        }

        ~ring() noexcept(noexcept(allocator_traits_t::deallocate(this->get_allocator(), this->get_composition().m_data, Size)) && noexcept(T().~T()))
        {
            allocator_t& alloc = this->get_allocator();
            composition_t& comp = this->get_composition();
            if (comp.m_data != nullptr)
            { allocator_traits_t::deallocate(alloc, comp.m_data, Size); }
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr size_t size(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_end - comp.m_begin;
        }
        constexpr bool is_full(this auto&& self)    noexcept { return self.size() == self.capacity(); }
        constexpr bool is_empty(this auto&& self)   noexcept { return self.size() == 0; }
        
        template <typename... Tys>
        constexpr bool try_emplace_back(Tys&&... args) noexcept(noexcept(T(std::forward<Tys>(args)...)))
        {
            allocator_t& alloc = this->get_allocator();
            composition_t& comp = this->get_composition();
            if (this->is_full()) { return false; }
            else
            {
                allocator_traits_t::construct(alloc, this->end(), std::forward<Tys>(args)...);
                this->inc_pos(comp.m_end);
                return true;
            }
        }
        constexpr bool try_push_back(const T& arg) noexcept(noexcept(this->try_emplace_back(arg))) { return this->try_emplace_back(arg); }
        constexpr bool try_push_back(T&& arg) noexcept(noexcept(this->try_emplace_back(std::move(arg)))) { return this->try_emplace_back(std::move(arg)); }
        constexpr void pop_back(this auto&& self) noexcept(noexcept(T().~T()))
        {
            auto& alloc = self.get_allocator();
            composition_t& comp = self.get_composition();
            if (!self.is_empty())
            {
                self.dec_pos(comp.m_end);
                allocator_traits_t::destroy(alloc, self.end());
            }
        }

        template <typename... Tys>
        constexpr bool try_emplace_front(Tys&&... args) noexcept(noexcept(T(std::forward<Tys>(args)...)))
        {
            allocator_t& alloc = this->get_allocator();
            composition_t& comp = this->get_composition();
            if (this->is_full()) { return false; }
            else
            {
                this->dec_pos(comp.m_begin);
                allocator_traits_t::construct(alloc, this->begin(), std::forward<Tys>(args)...);
                return true;
            }
        }
        constexpr bool try_push_front(const T& arg) noexcept(noexcept(this->try_emplace_front(arg))) { return this->try_emplace_front(arg); }
        constexpr bool try_push_front(T&& arg) noexcept(noexcept(this->try_emplace_front(std::move(arg)))) { return this->try_emplace_front(std::move(arg)); }
        constexpr void pop_front(this auto&& self) noexcept(noexcept(T().~T()))
        {
            allocator_t& alloc = self.get_allocator();
            composition_t& comp = self.get_composition();
            if (!self.is_empty())
            {
                allocator_traits_t::destroy(alloc, self.begin());
                self.inc_pos(comp.m_begin);
            }
        }

        [[nodiscard]]
        constexpr T& front() noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            return *(this->begin());
        }

        [[nodiscard]]
        constexpr const T& front() const noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            return *(this->begin());
        }

        [[nodiscard]]
        constexpr T& back() noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            return *(this->end() - 1);
        }

        [[nodiscard]]
        constexpr const T& back() const noexcept
        {
            assertm(!this->is_empty(), "Error : Get object from Zero size container(ring).");
            return *(this->end() - 1);
        }
    };
} // namespace sia
