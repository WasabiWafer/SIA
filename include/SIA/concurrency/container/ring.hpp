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
                    true_share<atomic_t> m_begin;
                    true_share<atomic_t> m_end;
                    true_share<T*> m_data;
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
                    constexpr std::pair<ring_counter_t, ring_counter_t> get_pair() noexcept
                    {
                        composition_t& comp = get_composition();
                        return {comp.m_begin->load(std::memory_order_relaxed), comp.m_end->load(std::memory_order_relaxed)};
                    }

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

                public:
                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair(splits::one_v, alloc)
                    {
                        composition_t& comp = get_composition();
                        comp.m_data = std::allocator_traits<allocator_type>::allocate(get_allocator(), capacity());
                    }

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
                enum class ring_object_state { ext_end = 0, ext_begin, emp_end, emp_begin, ext_wait, ext_notify, emp_wait, emp_notify };
                enum class ring_counter_state { none = 0, inc };

                struct state_composition
                {
                    true_share<state<ring_object_state>> m_object_state;
                    true_share<state<ring_counter_state>> m_beg_state;
                    true_share<state<ring_counter_state>> m_end_state;
                };

                template <typename T, size_t Size>
                struct ring_composition
                {
                    using ring_counter_t = sia::ring_detail::ring_counter<T, Size>;
                    using atomic_t = std::atomic<ring_counter_t>;
                    true_share<atomic_t> m_begin;
                    true_share<atomic_t> m_end;
                    true_share<atomic_t> m_total_begin;
                    true_share<atomic_t> m_total_end;
                    true_share<state_composition*> m_state;
                    true_share<T*> m_data;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::scoped_allocator_adaptor<std::allocator<T>, std::allocator<ring_detail::state_composition>>>
            struct ring
            {
                private:
                    using allocator_type = Allocator;
                    using composition_t = ring_detail::ring_composition<T, Size>;
                    using ring_counter_t = sia::ring_detail::ring_counter<size_t, Size>;
                    using ring_counter_value_t = size_t;
                    using state_composition_t = ring_detail::state_composition;
                    using object_state_value_type = ring_detail::ring_object_state;
                    using counter_state_value_type = ring_detail::ring_counter_state;
                    using object_state_type = state<object_state_value_type>;
                    using counter_state_type = state<counter_state_value_type>;

                    compressed_pair<allocator_type, composition_t> m_compair;

                    composition_t& get_composition() noexcept { return m_compair.second(); }
                    constexpr std::pair<ring_counter_t, ring_counter_t> get_push_counter_pair() noexcept
                    {
                        composition_t& comp = get_composition();
                        return {comp.m_total_begin->load(std::memory_order::relaxed), comp.m_end->load(std::memory_order::relaxed)};
                    }
                    constexpr std::pair<ring_counter_t, ring_counter_t> get_pop_counter_pair() noexcept
                    {
                        composition_t& comp = get_composition();
                        return {comp.m_begin->load(std::memory_order::relaxed), comp.m_total_end->load(std::memory_order::relaxed)};
                    }
                    size_t capacity() noexcept { return Size; }
                    constexpr T* address(size_t at) noexcept { return get_composition().m_data.ref() + at;}
                    template <typename... Tys>
                    constexpr void construct_at(T* at, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), at, std::forward<Tys>(args)...); }
                    constexpr void destruct_at(T* at) noexcept(std::is_nothrow_destructible_v<T>)
                    { std::allocator_traits<outer_allocator_type>::destroy(get_outer_allocator(), at); }
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

                    constexpr bool state_begin(object_state_type& obj_state, object_state_value_type comp0, object_state_value_type set0, object_state_value_type loop_out, object_state_value_type comp1, object_state_value_type set1) noexcept
                    {
                        if (obj_state.compare_exchange_strong(comp0, set0))
                        {
                            while (!(obj_state.status() == loop_out)) { }
                            obj_state.compare_exchange_strong(loop_out, set1);
                            return true;
                        }
                        else if (obj_state.compare_exchange_strong(comp1, set1))
                        { return true; }
                        else
                        { return false; }
                    }

                    constexpr void state_end(object_state_type& obj_state, object_state_value_type comp0, object_state_value_type set0, object_state_value_type comp1, object_state_value_type set1) noexcept
                    {
                        if (obj_state.compare_exchange_strong(comp0, set0))
                        { }
                        else
                        { obj_state.compare_exchange_strong(comp1, set1); }
                    }

                    constexpr void push_counter_inc(ring_counter_t counter, counter_state_type& counter_state) noexcept
                    {
                        composition_t& comp = get_composition();
                        ring_counter_t copy = counter;
                        if (!comp.m_end->compare_exchange_strong(copy, ring_counter_t{copy.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                        { counter_state.set(counter_state_value_type::inc); }
                        while (!comp.m_total_end->compare_exchange_weak(counter, ring_counter_t{counter.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                        { }
                    }

                    constexpr void pop_counter_inc(ring_counter_t counter, counter_state_type& counter_state) noexcept
                    {
                        composition_t& comp = get_composition();
                        ring_counter_t copy = counter;
                        if (!comp.m_begin->compare_exchange_strong(copy, ring_counter_t{copy.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                        { counter_state.set(counter_state_value_type::inc); }
                        while (!comp.m_total_begin->compare_exchange_weak(counter, ring_counter_t{counter.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                        { }
                    }

                public:
                    using outer_allocator_value_type = T;
                    using inner_allocator_value_type =  ring_detail::state_composition;
                    using outer_allocator_type = allocator_type::outer_allocator_type;
                    using inner_allocator_type = allocator_type::inner_allocator_type::outer_allocator_type;

                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair(splits::one_v, alloc)
                    {
                        composition_t& comp = get_composition();
                        comp.m_state = std::allocator_traits<inner_allocator_type>::allocate(get_inner_allocator(), capacity());
                        comp.m_data = std::allocator_traits<outer_allocator_type>::allocate(get_outer_allocator(), capacity());
                        for (size_t at{ }; at < capacity(); ++at)
                        {
                            std::allocator_traits<inner_allocator_type>::construct(get_inner_allocator(), comp.m_state.ref() + at);
                            std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), comp.m_data.ref() + at);
                        }
                    }

                    constexpr ~ring()
                    {
                        // impl
                    }

                    outer_allocator_type& get_outer_allocator() noexcept { return m_compair.first().outer_allocator(); }
                    inner_allocator_type& get_inner_allocator() noexcept { return m_compair.first().inner_allocator().outer_allocator(); }

                    template <typename... Tys>
                    constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {
                        composition_t& comp = get_composition();
                        for
                            (
                                auto pair {get_push_counter_pair()};
                                !is_full(pair.first.count(), pair.second.count());
                                pair.second.inc(),
                                pair.first = comp.m_total_begin->load(std::memory_order::relaxed)
                            )
                        {
                            object_state_type& obj_state = (comp.m_state.ref() + pair.second.offset())->m_object_state.ref();
                            counter_state_type& c_state = (comp.m_state.ref() + pair.second.offset())->m_end_state.ref();
                            if (state_begin(obj_state, object_state_value_type::ext_begin, object_state_value_type::ext_wait, object_state_value_type::ext_notify, object_state_value_type::ext_end, object_state_value_type::emp_begin))
                            {
                                construct_at(address(pair.second.offset()), std::forward<Tys>(args)...);
                                push_counter_inc(pair.second, c_state);
                                state_end(obj_state, object_state_value_type::emp_begin, object_state_value_type::emp_end, object_state_value_type::emp_wait, object_state_value_type::emp_notify);
                                return true;
                            }
                            else
                            {
                                if (c_state.status() == counter_state_value_type::inc)
                                {
                                    ring_counter_t copy = pair.second;
                                    if (comp.m_end->compare_exchange_strong(copy, ring_counter_t{copy.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                                    { c_state.set(counter_state_value_type::none); }
                                }
                            }
                        }
                        return false;
                    }

                    template <typename Ty>
                        requires (std::is_assignable_v<Ty, T&> || std::is_assignable_v<Ty, T&&>)
                    constexpr bool try_extract_front(Ty&& arg)
                        noexcept
                        (
                            (std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>)
                        )
                    {
                        composition_t& comp = get_composition();
                        for
                            (
                                auto pair {get_pop_counter_pair()};
                                !is_empty(pair.first.count(), pair.second.count());
                                pair.first.inc(),
                                pair.second = comp.m_total_end->load(std::memory_order::relaxed)
                            )
                        {
                            object_state_type& obj_state = (comp.m_state.ref() + pair.first.offset())->m_object_state.ref();
                            counter_state_type& c_state = (comp.m_state.ref() + pair.first.offset())->m_end_state.ref();
                            if (state_begin(obj_state, object_state_value_type::emp_begin, object_state_value_type::emp_wait, object_state_value_type::emp_notify, object_state_value_type::emp_end, object_state_value_type::ext_begin))
                            {
                                return true;
                            }
                            else
                            {
                                if (c_state.status() == counter_state_value_type::inc)
                                {
                                    ring_counter_t copy = pair.first;
                                    if (comp.m_end->compare_exchange_strong(copy, ring_counter_t{copy.next()}, std::memory_order::relaxed, std::memory_order::relaxed))
                                    { c_state.set(counter_state_value_type::none); }
                                }
                            }
                        }
                        return false;
                    }
                };
            } // namespace mpmc
        } // namespace concurrency
    } // namespace sia