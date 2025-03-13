#pragma once

#include <utility>
#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace lane_detail
    {
        template <typename T>
        struct lane_composition
        {
            T* m_data;
            T* m_end;
        };
    } // namespace lane_detail
    
    template <typename T, size_t Size, typename Allocator = std::allocator<T>>
    struct lane
    {
    private:
        using composition_t = lane_detail::lane_composition<T>;
        using allocator_t = Allocator;
        using allocator_traits_t = std::allocator_traits<Allocator>;

        compressed_pair<Allocator, composition_t> m_compair;

        constexpr allocator_t& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr composition_t& get_composition(this auto&& self) noexcept { return self.m_compair.second(); }
        constexpr T* address(this auto&& self, size_t idx) noexcept { return self.get_composition().m_data + idx; }
        
    public:
        constexpr lane(const allocator_t& alloc = allocator_t()) noexcept(noexcept(allocator_t(alloc)) && noexcept(allocator_traits_t::allocate(this->get_allocator(), this->capacity())))
            : m_compair(splits::one_v, alloc, nullptr, nullptr)
        {
            allocator_t& allocator = this->get_allocator();
            composition_t& comp = this->get_composition();
            comp.m_data = allocator_traits_t::allocate(allocator, this->capacity());
            comp.m_end = comp.m_data;
        }

        ~lane() noexcept(noexcept(allocator_traits_t::deallocate(this->get_allocator(), this->get_composition().m_data, this->capacity())) && noexcept(T().~T()))
        {
            allocator_t& alloc = this->get_allocator();
            composition_t& comp = this->get_composition();
            allocator_traits_t::deallocate(alloc, comp.m_data, this->capacity());
        }

        constexpr T* begin() noexcept
        {
            composition_t& comp = this->get_composition();
            return comp.m_data;
        }
        constexpr T* end() noexcept
        {
            composition_t& comp = this->get_composition();
            return comp.m_end;
        }
        constexpr size_t size(this auto&& self)     noexcept { return self.end() - self.begin(); }
        constexpr size_t capacity(this auto&& self) noexcept { return Size; }
        constexpr bool is_full(this auto&& self)    noexcept { return self.size() == self.capacity(); }
        constexpr bool is_empty(this auto&& self)   noexcept { return self.size() == 0; }

        [[nodiscard]]
        constexpr T& operator[](this auto&& self, size_t idx) noexcept
        {
            assertm(idx < self.size(), "Error : access violation");
            return *self.address(idx);
        }

        template <typename... Tys>
        constexpr bool try_emplace_back(Tys&&... args) noexcept(noexcept(T(std::forward<Tys>(args)...)))
        {
            allocator_t& allocator = this->get_allocator();
            composition_t& comp = this->get_composition();
            if (this->is_full()) { return false; }
            else
            {
                allocator_traits_t::construct(allocator, comp.m_end, std::forward<Tys>(args)...);
                ++comp.m_end;
                return true;
            }
        }
        constexpr bool try_push_back(const T& arg) noexcept(noexcept(this->try_emplace_back(arg))) { return this->try_emplace_back(arg); }
        constexpr bool try_push_back(T&& arg) noexcept(noexcept(this->try_emplace_back(std::move(arg)))) { return this->try_emplace_back(std::move(arg)); }
        constexpr void pop_back(this auto&& self) noexcept(noexcept(T().~T()))
        {
            allocator_t& alloc = self.get_allocator();
            composition_t& comp = self.get_composition();
            if (!self.is_empty())
            {
                allocator_traits_t::destroy(alloc, --comp.m_end);
            }
        }

        [[nodiscard]]
        constexpr T& back() noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_end - 1);
        }

        [[nodiscard]]
        constexpr const T& back() const noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_end - 1);
        }
    };
} // namespace sia
