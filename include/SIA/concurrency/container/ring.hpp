#pragma once

#include <memory>
#include <scoped_allocator>
#include <atomic>

#include "SIA/container/ring.hpp"

#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/utility/tools.hpp"
#include "SIA/concurrency/utility/state.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace spsc
        {
            namespace ring_detail
            {
                template <typename T, size_t Size>
                    requires (std::atomic<sia::ring_detail::ring_counter<size_t, Size>>::is_always_lock_free)
                struct ring_composition
                {
                    using ring_counter_t = sia::ring_detail::ring_counter<size_t, Size>;
                    using atomic_t = std::atomic<ring_counter_t>;
                    false_share<atomic_t> m_begin;
                    false_share<atomic_t> m_end;
                    false_share<T*> m_data;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::allocator<T>>
            struct ring
            {
                private:
                    using ring_counter_t = sia::ring_detail::ring_counter<size_t, Size>;
                    using allocator_type = Allocator;
                    using composition_t = ring_detail::ring_composition<T, Size>;

                    compressed_pair<allocator_type, composition_t> m_compair;
                
                    constexpr composition_t& get_composition() noexcept { return m_compair.second(); }

                    constexpr T* address(size_t at) noexcept { return get_composition().m_data.ref() + at;}

                    template <typename... Tys>
                    constexpr void construct_at(T* at, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { std::allocator_traits<allocator_type>::construct(get_allocator(), at, std::forward<Tys>(args)...); }

                    constexpr void destruct_at(T* at) noexcept(std::is_nothrow_destructible_v<T>)
                    { std::allocator_traits<allocator_type>::destroy(get_allocator(), at); }

                    constexpr size_t size(size_t beg, size_t end) noexcept
                    {
                        constexpr const size_t adj = ring_counter_t::adjustment();
                        if (end >= beg)
                        { return end - beg; }
                        else
                        { return end - beg - adj; }
                    }
                    constexpr bool is_empty(size_t beg, size_t end) noexcept { return size(beg, end) == 0; }
                    constexpr bool is_full(size_t beg, size_t end) noexcept { return size(beg, end) == Size; }

                    constexpr std::pair<ring_counter_t, ring_counter_t> get_pair() noexcept
                    {
                        composition_t& comp = get_composition();
                        return {comp.m_begin->load(std::memory_order_relaxed), comp.m_end->load(std::memory_order_relaxed)};
                    }

                public:
                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair(splits::one_v, alloc)
                    { get_composition().m_data = std::allocator_traits<allocator_type>::allocate(get_allocator(), capacity()); }

                    constexpr ~ring()
                    { std::allocator_traits<allocator_type>::deallocate(get_allocator(), address(0), capacity()); }

                    constexpr allocator_type& get_allocator() noexcept { return m_compair.first(); }
                    constexpr size_t capacity() noexcept { return Size; }
                    constexpr size_t size() noexcept
                    {
                        composition_t& comp = get_composition();
                        constexpr const size_t adj = ring_counter_t::adjustment();
                        size_t beg_count = comp.m_begin->load(std::memory_order::relaxed).count();
                        size_t end_count = comp.m_end->load(std::memory_order::relaxed).count();
                        if (end_count >= beg_count)
                        { return end_count - beg_count; }
                        else
                        { return end_count - beg_count - adj; }
                    }
                    constexpr bool is_empty() noexcept { return size() == 0; }
                    constexpr bool is_full() noexcept { return size() == capacity(); }

                    template <typename... Tys>
                    constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {
                        auto pair = get_pair();
                        if (!is_full(pair.first.count(), pair.second.count()))
                        {
                            construct_at(address(pair.second.offset()), std::forward<Tys>(args)...);
                            pair.second.inc();
                            get_composition().m_end->store(pair.second, std::memory_order::relaxed);
                            return true;
                        }
                        return false;
                    }
                    constexpr bool try_push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                    { return try_emplace_back(arg); }
                    constexpr bool try_push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                    { return try_emplace_back(std::move(arg)); }
                    
                    template <typename Ty>
                        requires (std::is_assignable_v<Ty, T&> || std::is_assignable_v<Ty, T&&>)
                    constexpr bool try_extract_front(Ty&& arg)
                        noexcept
                        (
                            (std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>)
                        )
                    {
                        auto pair = get_pair();
                        if (!is_empty(pair.first.count(), pair.second.count()))
                        {
                            T* target = address(pair.first.offset());
                            if constexpr (std::is_assignable_v<Ty, T&&>) { arg = std::move(*target); }
                            else { arg = *target; }
                            destruct_at(target);
                            pair.first.inc();
                            get_composition().m_begin->store(pair.first, std::memory_order::relaxed);
                            return true;
                        }
                        return false;
                    }

                    constexpr bool try_pull_front(T& arg)
                        noexcept
                        (
                            (std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                            (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>)
                        )
                    { return try_extract_front(arg); }

                    template <tags::loop LoopTag, tags::wait WaitTag, typename... Tys>
                    constexpr bool loop_emplace_back(auto ltt_v, auto wtt_v, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &ring::try_emplace_back<Tys...>, this, std::forward<Tys>(args)...); }

                    template <tags::loop LoopTag, tags::wait WaitTag, typename Ty>
                    constexpr bool loop_extract_front(auto ltt_v, auto wtt_v, Ty&& arg)
                        noexcept
                        (
                            (std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>)
                        )
                    { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &ring::try_extract_front<Ty>, this, std::forward<Ty>(arg));}

                    template <typename... Tys>
                    constexpr void emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Tys>(args)...); }
                    constexpr void push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
                    constexpr void push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::move(arg)); }

                    template <typename Ty>
                    constexpr void extract_front(Ty&& arg)
                        noexcept
                        (
                            (std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>)
                        )
                    { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Ty>(arg)); }

                    constexpr void pull_front(T& arg)
                        noexcept
                        (
                            (std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                            (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>)
                        )
                    { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
            };
        } // namespace spsc
        
        namespace mpmc
        {
            
            namespace ring_detail
            {
                enum class ring_object_state {  };

                template <typename T, size_t Size>
                struct ring_composition
                {
                    using ring_counter_t = sia::ring_detail::ring_counter<T, Size>;
                    using atomic_t = std::atomic<ring_counter_t>;
                    false_share<atomic_t> m_begin;
                    false_share<atomic_t> m_end;
                    false_share<ring_object_state*> m_state;
                    false_share<T*> m_data;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::scoped_allocator_adaptor<std::allocator<T>, std::allocator<ring_detail::ring_object_state>>>
            struct ring
            {
                private:
                    using allocator_type = Allocator;
                    using composition_t = ring_detail::ring_composition<T, Size>;
                    using ring_counter_t = sia::ring_detail::ring_counter<size_t, Size>;

                    compressed_pair<allocator_type, composition_t> m_compair;

                    constexpr composition_t& get_composition() noexcept { return m_compair.second(); }

                public:
                    using outer_allocator_value_type = T;
                    using inner_allocator_value_type = ring_detail::ring_object_state;
                    using outer_allocator_type = allocator_type::outer_allocator_type;
                    using inner_allocator_type = allocator_type::inner_allocator_type;

                    constexpr outer_allocator_type& get_outer_allocator() noexcept { return m_compair.first().outer_allocator(); }
                    constexpr inner_allocator_type& get_inner_allocator() noexcept { return m_compair.first().inner_allocator(); }
                    constexpr allocator_type& get_allocator() noexcept { return m_compair.first(); }
                    constexpr size_t capacity() noexcept { return Size; }

                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair{splits::one_v, alloc}
                    {
                        composition_t& comp = get_composition();
                        comp.m_state = std::allocator_traits<inner_allocator_type>::allocate(get_inner_allocator(), capacity());
                        comp.m_data = std::allocator_traits<outer_allocator_type>::allocate(get_outer_allocator(), capacity());
                    }

                    constexpr ~ring()
                    {
                        composition_t& comp = get_composition();
                        
                    }

                    template <typename... Tys>
                    constexpr bool try_emplace_back() noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {

                        return false;
                    }
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia