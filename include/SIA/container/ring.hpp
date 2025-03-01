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

        constexpr Allocator& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
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
    public:
        constexpr ring(const Allocator& alloc = Allocator())
            : m_compair(splits::one_v, alloc, nullptr, 0, 0)
        {
            auto& allocator = this->get_allocator();
            composition_t& comp = this->get_composition();
            comp.m_data = allocator_traits_t::allocate(allocator, Size);
        }

        constexpr ring(const std::initializer_list<T>& arg, const Allocator& alloc = Allocator())
            : m_compair(splits::one_v, alloc, nullptr, 0, 0)
        {
            assertm(arg.size() <= Size, "Error : initialize with oversize.");
            auto& allocator = this->get_allocator();
            composition_t& comp = this->get_composition();
            comp.m_data = allocator_traits_t::allocate(allocator, Size);
            for (auto& elem : arg)
            {
                allocator_traits_t::construct(allocator, comp.m_data + comp.m_end, elem);
                ++comp.m_end;
            }
        }

        ~ring()
        {
            auto& allocator = this->get_allocator();
            composition_t& comp = this->get_composition();
            if (comp.m_data != nullptr)
            { allocator_traits_t::deallocate(allocator, comp.m_data, Size); }
        }

        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr size_t size(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_end - comp.m_begin;
        }
        constexpr bool is_full(this auto&& self)    noexcept { return self.size() == self.capacity(); }
        constexpr bool is_empty(this auto&& self)   noexcept { return self.size() == 0; }

        [[nodiscard]]
        constexpr T& at(this auto&& self, size_t idx) noexcept { return *self.address(idx); }
        [[nodiscard]]
        constexpr T& operator[](this auto&& self, size_t idx) noexcept { return self.at(idx); }
        
        template <typename... Tys>
        constexpr bool try_emplace_back(Tys&&... args)
        {
            if (this->is_full()) { return false; }
            else
            {
                auto& alloc = this->get_allocator();
                composition_t& comp = this->get_composition();
                allocator_traits_t::construct(alloc, this->end(), std::forward<Tys>(args)...);
                ++comp.m_end;
                return true;
            }
        }
        constexpr bool try_push_back(const T& arg)  { return this->try_emplace_back(arg); }
        constexpr bool try_push_back(T&& arg)       { return this->try_emplace_back(std::move(arg)); }

        template <typename... Tys>
        constexpr bool try_emplace_front(Tys&&... args)
        {
            if (this->is_full()) { return false; }
            else
            {
                auto& alloc = this->get_allocator();
                composition_t& comp = this->get_composition();
                --comp.m_begin;
                allocator_traits_t::construct(alloc, this->begin(), std::forward<Tys>(args)...);
                return true;
            }
        }
        constexpr bool try_push_front(const T& arg)  { return this->try_emplace_front(arg); }
        constexpr bool try_push_front(T&& arg)       { return this->try_emplace_front(std::move(arg)); }

        constexpr void pop_front(this auto&& self)
        {
            auto& alloc = self.get_allocator();
            composition_t& comp = self.get_composition();
            if (!self.is_empty())
            {
                allocator_traits_t::destroy(alloc, self.begin());
                ++comp.m_begin;
            }
        }

        constexpr void pop_back(this auto&& self)
        {
            auto& alloc = self.get_allocator();
            composition_t& comp = self.get_composition();
            if (!self.is_empty())
            {
                --comp.m_end;
                allocator_traits_t::destroy(alloc, self.end());
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
