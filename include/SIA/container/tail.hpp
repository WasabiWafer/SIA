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
            constexpr T* data_ptr(this auto&& self) noexcept { return self.m_data; }
            constexpr tail_node& tail(this auto&& self) noexcept { return *(self.m_tail); }
            constexpr tail_node* tail_ptr(this auto&& self) noexcept { return self.m_tail; }
            constexpr tail_node& operator++(this auto&& self) noexcept { return self.tail(); }
            constexpr tail_node operator++(this auto&& self, int) noexcept { return self.tail(); }
        };
    } // namespace tail_detail
    
    // change to pmr
    template <typename T, typename Allocator = std::pmr::polymorphic_allocator<>>
    struct tail {
    private:
        using node_t = tail_detail::tail_node<T>;
        compressed_pair<Allocator, node_t*> compair;
    public:
        constexpr tail(const Allocator& alloc = Allocator()) noexcept
            : compair(compressed_pair_tag::one, alloc)
        { }
        
        constexpr node_t* begin() noexcept { return compair.second(); }

        constexpr node_t* end() noexcept { return nullptr; }

        constexpr Allocator& get_allocator() noexcept { return compair.first(); }

        template <typename C>
        constexpr void push_front(C&& arg) noexcept { emplace_front(std::forward<C>(arg)); }

        template <typename... Cs>
        constexpr void emplace_front(Cs&&... args) noexcept
        {
            Allocator& alloc = get_allocator();
            node_t* gen_node = alloc.allocate_object<node_t>(1);
            T* gen_data = alloc.allocate_object<T>(1);
            alloc.construct(gen_data, std::forward<Cs>(args)...);
            alloc.construct(gen_node, gen_data, begin());
            compair.second() = gen_node;
        }

        template <typename C>
        constexpr bool pop_front(C&& arg) noexcept
        {
            Allocator& alloc = get_allocator();
            node_t* out_node = begin();
            if (out_node == nullptr) { return false; }
            compair.second() = out_node->tail_ptr();
            arg = std::move(out_node->data());
            alloc.delete_object(out_node->data_ptr());
            alloc.deallocate_object(out_node, 1);
            return true;
        }
    };
} // namespace sia
