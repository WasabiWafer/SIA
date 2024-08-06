#pragma once

#include <memory>

#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace tail_detail
    {
        template <typename T>
        struct tail_node {
        private:
            tail_node* m_tail;
            T* m_data;
        public:
            constexpr tail_node() noexcept : m_data(), m_tail() { }
            constexpr tail_node(T* data_ptr, tail_node* tail_ptr = nullptr) noexcept : m_data(data_ptr), m_tail(tail_ptr) { }
            constexpr T& data(this auto&& self) noexcept { return *(self.m_data); }
            constexpr tail_node& tail(this auto&& self) noexcept { return *(self.m_tail); }
            constexpr tail_node& operator++(this auto&& self) noexcept { return self.tail(); }
            constexpr tail_node operator++(this auto&& self, int) noexcept { return self.tail(); }
        };
    } // namespace tail_detail
    
    // change to pmr
    template <typename T, typename Allocator = std::allocator<T>>
    struct tail {
    private:
        using node_t = tail_detail::tail_node<T>;
        using data_allocator_traits_t = std::allocator_traits<Allocator>;
        using node_allocator = data_allocator_traits_t::template rebind_alloc<node_t>;
        using node_allocator_traits_t = std::allocator_traits<node_allocator>;
        compressed_pair<Allocator, node_t*> compair;
    public:
        constexpr tail(const Allocator& alloc = Allocator()) noexcept
            : compair(compressed_pair_tag::one, alloc)
        { }
        
        constexpr node_t* begin() noexcept { return compair.second(); }
        constexpr node_t* end() noexcept { return nullptr; }
        
        template <typename C>
        constexpr void push_front(C&& arg) noexcept { emplace_front(std::forward<C>(arg)); }

        template <typename... Cs>
        constexpr void emplace_front(Cs&&... args) noexcept
        {
            T* gen_data = data_allocator_traits_t::allocate(compair.first(), 1);
            data_allocator_traits_t::construct(compair.first(), gen_data, std::forward<Cs>(args)...);
            node_allocator node_alloc {compair.first()};
            node_t* gen_node = node_allocator_traits_t::allocate(node_alloc, 1);
            node_allocator_traits_t::construct(node_alloc, gen_node, gen_data, begin());
            compair.second() = gen_node;
        }

        template <typename C>
        constexpr void pop_front() noexcept
        {

        }
    };
} // namespace sia
