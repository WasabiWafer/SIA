#pragma once

#include <limits>
#include <memory>
#include <iterator>

#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace ring_detail
    {
        template <typename T, T Size>
        struct ring_counter
        {
                using counter_type = T;
                counter_type m_counter;

                static constexpr counter_type adjustment() noexcept
                { return (std::numeric_limits<counter_type>::max() % Size) + 1; }
                constexpr void inc() noexcept
                {
                    if (count() == std::numeric_limits<counter_type>::max())
                    { add(adjustment() + 1); }
                    else
                    { add(1); }
                }

                constexpr void dec() noexcept
                {
                    if (count() == std::numeric_limits<counter_type>::min())
                    { sub(adjustment() + 1); }
                    else
                    { sub(1); }
                }
                
                constexpr void add(const counter_type& arg) noexcept { m_counter += arg; }
                constexpr void sub(const counter_type& arg) noexcept { m_counter -= arg; }
                constexpr counter_type offset() const noexcept { return m_counter % Size; }
                constexpr counter_type count() const noexcept { return m_counter; }
                constexpr counter_type next() const noexcept
                {
                    ring_counter ret {*this};
                    ret.inc();
                    return ret.count();
                }
                constexpr counter_type prev() const noexcept
                {
                    ring_counter ret {*this};
                    ret.dec();
                    return ret.count();
                }
        };

        template <typename T, size_t Size>
        struct ring_composition
        {
            T* m_data;
            ring_counter<size_t, Size> m_begin;
            ring_counter<size_t, Size> m_end;
        };

        template <typename T, size_t Size>
        struct ring_iterator
        {
            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using diff_type = std::ptrdiff_t;
                using value_type = T;
                using pointer = T*;
                using reference = T&;

            private:
                T* m_data;
                ring_counter<size_t, Size> m_count;

                constexpr pointer ptr() noexcept { return (m_data + m_count.offset()); }
                constexpr const pointer ptr() const noexcept { return (m_data + m_count.offset()); }

            public:
                constexpr ring_iterator(T* data_ptr, size_t begin_count) noexcept
                    : m_data(data_ptr), m_count(begin_count)
                { }

                constexpr reference operator*() noexcept { return *ptr(); }
                constexpr const reference operator*() const noexcept { return *ptr(); }
                constexpr pointer operator->() noexcept { return ptr(); }
                constexpr const pointer operator->() const noexcept { return ptr(); }
                constexpr ring_iterator& operator++() noexcept
                {
                    m_count.inc();
                    return *this;
                }
                constexpr ring_iterator operator++(int) noexcept
                {
                    ring_iterator ret = *this;
                    m_count.inc();
                    return ret;
                }

                constexpr ring_iterator& operator--() noexcept
                {
                    m_count.dec();
                    return *this;
                }
                constexpr ring_iterator operator--(int) noexcept
                {
                    ring_iterator ret = *this;
                    m_count.dec();
                    return ret;
                }
                // +, -, +=, -= ptrdiff type(diff_type)

                friend bool operator==(const ring_iterator& arg0, const ring_iterator& arg1) noexcept { return arg0.ptr() == arg1.ptr(); };
                friend bool operator!=(const ring_iterator& arg0, const ring_iterator& arg1) noexcept { return arg0.ptr() != arg1.ptr(); };
        };
    } // namespace ring_detail
    
    template <typename T, size_t Size, typename Allocator = std::allocator<T>>
    struct ring
    {
        private:
            using ring_counter_t = ring_detail::ring_counter<size_t, Size>;
            using composition_t = ring_detail::ring_composition<T, Size>;
            using allocator_type = Allocator;
            using iterator_type = ring_detail::ring_iterator<T, Size>;

            compressed_pair<allocator_type, composition_t> m_compair;

            constexpr composition_t& get_composition() noexcept { return m_compair.second(); }
            
            template <typename... Tys>
            constexpr void construct_at(T* at, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
            { std::allocator_traits<allocator_type>::construct(get_allocator(), at, std::forward<Tys>(args)...); }
            
            constexpr void destruct_at(T* at) noexcept(std::is_nothrow_destructible_v<T>)
            { std::allocator_traits<allocator_type>::destroy(get_allocator(), at); }
            
            constexpr T* address(size_t count) noexcept { return get_composition().m_data + count; }
            
        public:
            constexpr ~ring() noexcept(std::is_nothrow_destructible_v<T>)
            {
                composition_t& comp = get_composition();
                if (comp.m_data != nullptr)
                { std::allocator_traits<allocator_type>::deallocate(get_allocator(), comp.m_data, capacity()); }
            }

            constexpr ring(const allocator_type& alloc = allocator_type{ }) 
                : m_compair(splits::one_v, alloc)
            { get_composition().m_data = std::allocator_traits<allocator_type>::allocate(get_allocator(), capacity()); }

            constexpr ring(ring& arg, const allocator_type& alloc = allocator_type{ })
                : m_compair(splits::one_v, alloc)
            {
                get_composition().m_data = std::allocator_traits<allocator_type>::allocate(get_allocator(), capacity());
                for (auto& elem : arg)
                { emplace_back(elem); }
            }

            constexpr ring(ring&& arg, const allocator_type& alloc = allocator_type{ }) noexcept(std::is_nothrow_constructible_v<allocator_type, allocator_type&&>)
                : m_compair(splits::one_v, alloc)
            {
                composition_t& comp = get_composition();
                composition_t& target_comp = arg.get_composition();
                comp.m_data = target_comp.m_data;
                target_comp.m_data = nullptr;
                comp.m_begin = target_comp.m_begin;
                comp.m_end = target_comp.m_end;
            }

            // constexpr ring& operator=(ring& arg) noexcept(std::is_nothrow_constructible_v<T, T&>)
            // constexpr ring& operator=(ring&& arg)

            template <typename... Tys>
            constexpr void emplace_front(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
            {
                composition_t& comp = get_composition();
                comp.m_begin.dec();
                this->construct_at(address(comp.m_begin.offset()), std::forward<Tys>(args)...);
            }

            template <typename... Tys>
            constexpr void emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
            {
                composition_t& comp = get_composition();
                this->construct_at(address(comp.m_end.offset()), std::forward<Tys>(args)...);
                comp.m_end.inc();
            }

            constexpr void push_front(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
            { emplace_front(arg); }
            constexpr void push_front(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
            { emplace_front(std::move(arg)); }

            constexpr void push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
            { emplace_back(arg); }
            constexpr void push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
            { emplace_back(std::move(arg)); }

            constexpr void pop_front() noexcept(std::is_nothrow_destructible_v<T>)
            {
                composition_t& comp = get_composition();
                this->destruct_at(address(comp.m_begin.offset()));
                comp.m_begin.inc();
            }

            constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<T>)
            {
                composition_t& comp = get_composition();
                comp.m_end.dec();
                this->destruct_at(address(comp.m_end.offset()));
            }

            constexpr size_t capacity() noexcept { return Size; }
            constexpr size_t size() noexcept
            {
                constexpr const size_t adj = ring_counter_t::adjustment();
                composition_t& comp = get_composition();
                size_t beg_count = comp.m_begin.count();
                size_t end_count = comp.m_end.count();
                if (end_count >= beg_count)
                { return end_count - beg_count; }
                else
                { return (end_count - beg_count) - adj; }
            }
            constexpr bool is_empty() noexcept { return size() == 0; }
            constexpr bool is_full() noexcept { return size() == capacity(); }

            iterator_type begin() noexcept
            {
                composition_t& comp = get_composition();
                return {comp.m_data, comp.m_begin.count()};
            }

            iterator_type end() noexcept
            {
                composition_t& comp = get_composition();
                return {comp.m_data, comp.m_end.count()};
            }

            constexpr allocator_type& get_allocator() noexcept { return m_compair.first(); }
    };
} // namespace sia
