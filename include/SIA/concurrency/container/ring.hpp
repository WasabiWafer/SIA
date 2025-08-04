#pragma once

#include <memory>
#include <scoped_allocator>
#include <atomic>

#include "SIA/container/ring.hpp"
#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/utility/state.hpp"
#include "SIA/concurrency/utility/tools.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace spsc
        {
            namespace ring_detail
            {
                template <typename T>
                struct ring_base;
                template <typename T, size_t Size, typename Allocator, template <typename, size_t, typename> typename RingType>
                struct ring_base<RingType<T, Size, Allocator>>
                {
                    private:
                        using derived_type = RingType<T, Size, Allocator>;
                        using ring_counter_t = sia::ring_detail::ring_counter<size_t, Size>;
                        constexpr ring_base() noexcept = default;
                        constexpr ~ring_base() noexcept = default;
                        friend derived_type;

                        static constexpr size_t size(size_t beg, size_t end) noexcept
                        {
                            constexpr const size_t adj = ring_counter_t::adjustment();
                            if (end >= beg)
                            { return end - beg; }
                            else
                            { return end - beg - adj; }
                        }
                        static constexpr bool is_empty(size_t beg, size_t end) noexcept { return size(beg, end) == 0; }
                        static constexpr bool is_full(size_t beg, size_t end) noexcept { return size(beg, end) == Size; }

                    public:
                        static constexpr size_t capacity() noexcept { return Size; }

                        template <tags::loop LoopTag, tags::wait WaitTag, typename... Tys>
                        constexpr bool loop_emplace_back(auto ltt_v, auto wtt_v, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                        { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &derived_type::template try_emplace_back<Tys...>, static_cast<derived_type*>(this), std::forward<Tys>(args)...); }
                        template <typename... Tys>
                        constexpr void emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                        { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Tys>(args)...); }
                        constexpr bool try_push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                        { return static_cast<derived_type*>(this)->try_emplace_back(arg); }
                        constexpr bool try_push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                        { return static_cast<derived_type*>(this)->try_emplace_back(std::move(arg)); }
                        constexpr void push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                        { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
                        constexpr void push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                        { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::move(arg)); }

                        template <tags::loop LoopTag, tags::wait WaitTag, typename Ty>
                        constexpr bool loop_extract_front(auto ltt_v, auto wtt_v, Ty&& arg)
                            noexcept
                            (
                                std::is_nothrow_destructible_v<T> &&
                                ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                                (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                            )
                        { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &derived_type::template try_extract_front<Ty>, static_cast<derived_type*>(this), std::forward<Ty>(arg));}
                        template <typename Ty>
                        constexpr void extract_front(Ty&& arg)
                            noexcept
                            (
                                std::is_nothrow_destructible_v<T> &&
                                ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                                (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                            )
                        { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Ty>(arg)); }
                        constexpr bool try_pull_front(T& arg)
                            noexcept
                            (
                                std::is_nothrow_destructible_v<T> &&
                                ((std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                                (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>))
                            )
                        { return static_cast<derived_type*>(this)->try_extract_front(arg); }
                        constexpr void pull_front(T& arg)
                            noexcept
                            (
                                std::is_nothrow_destructible_v<T> &&
                                ((std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                                (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>))
                            )
                        { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
                };

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
            struct ring : public ring_detail::ring_base<ring<T, Size, Allocator>>
            {
                private:
                    using base_type = ring_detail::ring_base<ring<T, Size, Allocator>>;
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

                public:
                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair(splits::one_v, alloc)
                    {
                        composition_t& comp = get_composition();
                        comp.m_data = std::allocator_traits<allocator_type>::allocate(get_allocator(), base_type::capacity());
                    }

                    constexpr ~ring()
                    { std::allocator_traits<allocator_type>::deallocate(get_allocator(), address(0), base_type::capacity()); }

                    constexpr allocator_type& get_allocator() noexcept { return m_compair.first(); }
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
                    constexpr bool is_full() noexcept { return size() == base_type::capacity(); }
                    
                    template <typename... Tys>
                    constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {
                        auto pair = get_pair();
                        if (!base_type::is_full(pair.first.count(), pair.second.count()))
                        {
                            construct_at(address(pair.second.offset()), std::forward<Tys>(args)...);
                            pair.second.inc();
                            get_composition().m_end->store(pair.second, std::memory_order::relaxed);
                            return true;
                        }
                        return false;
                    }

                    template <typename Ty>
                        requires (std::is_assignable_v<Ty, T&> || std::is_assignable_v<Ty, T&&>)
                    constexpr bool try_extract_front(Ty&& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                        )
                    {
                        auto pair = get_pair();
                        if (!base_type::is_empty(pair.first.count(), pair.second.count()))
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
            };
        } // namespace spsc
        
        namespace mpmc
        {
            
            namespace ring_detail
            {
                enum class ring_action_state { poped = 0, pushed };

                struct state_composition
                {
                    // true_share<std::atomic<ring_action_state>> m_last_action_state;
                    true_share<state<ring_action_state>> m_last_action_state;
                    true_share<std::atomic<size_t>> m_input_state;
                    true_share<std::atomic<size_t>> m_output_state;
                };

                template <typename T, size_t Size>
                struct ring_composition
                {
                    using ring_counter_t = sia::ring_detail::ring_counter<T, Size>;
                    using atomic_t = std::atomic<ring_counter_t>;
                    true_share<atomic_t> m_begin;
                    true_share<atomic_t> m_end;
                    true_share<state_composition*> m_state;
                    true_share<T*> m_data;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::scoped_allocator_adaptor<std::allocator<T>, std::allocator<ring_detail::state_composition>>>
                requires (Size <= (std::numeric_limits<size_t>::max()/2))
            struct ring : public sia::concurrency::spsc::ring_detail::ring_base<ring<T, Size, Allocator>>
            {
                private:
                    using base_type = sia::concurrency::spsc::ring_detail::ring_base<ring<T, Size, Allocator>>;
                    using allocator_type = Allocator;
                    using composition_t = ring_detail::ring_composition<T, Size>;
                    using ring_counter_value_t = size_t;
                    using ring_counter_t = sia::ring_detail::ring_counter<ring_counter_value_t, Size>;
                    using ring_counter_atomic_t = std::atomic<ring_counter_t>;
                    using state_type = std::atomic<size_t>;
                    using action_state_value_t = ring_detail::ring_action_state;
                    using action_state_type = state<action_state_value_t>;

                    compressed_pair<allocator_type, composition_t> m_compair;

                    composition_t& get_composition() noexcept { return m_compair.second(); }
                    constexpr std::pair<ring_counter_t, ring_counter_t> get_pair() noexcept
                    {
                        composition_t& comp = get_composition();
                        return {comp.m_begin->load(std::memory_order::relaxed), comp.m_end->load(std::memory_order::relaxed)};
                    }

                    constexpr T* address(size_t at) noexcept { return get_composition().m_data.ref() + at;}
                    template <typename... Tys>
                    constexpr void construct_at(T* at, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), at, std::forward<Tys>(args)...); }
                    constexpr void destruct_at(T* at) noexcept(std::is_nothrow_destructible_v<T>)
                    { std::allocator_traits<outer_allocator_type>::destroy(get_outer_allocator(), at); }

                    constexpr void counter_inc(ring_counter_atomic_t& target_atomic_count, ring_counter_t init = ring_counter_t{ }) noexcept
                    { while (!target_atomic_count.compare_exchange_weak(init, ring_counter_t{init.next()}, std::memory_order::relaxed, std::memory_order::relaxed)) { } }

                    constexpr bool state_enter(state_type& state, ring_counter_t counter) noexcept
                    {
                        size_t target_count = counter.count();
                        if (target_count == state.load(std::memory_order::relaxed))
                        { return state.compare_exchange_strong(target_count, counter.next_cycle(), std::memory_order_relaxed, std::memory_order_relaxed); }
                        else
                        { return false; }
                    }
                    constexpr void state_wait(action_state_type& state, action_state_value_t expt) noexcept
                    { while(!(expt == state.status())) { } }

                    constexpr bool double_check_is_full(auto& pair) noexcept
                    {
                        if(base_type::is_full(pair.first.count(), pair.second.count()))
                        {
                            pair = get_pair();
                            return base_type::is_full(pair.first.count(), pair.second.count());
                        }
                        return false;
                    }
                    constexpr bool double_check_is_empty(auto& pair) noexcept
                    {
                        if(base_type::is_empty(pair.first.count(), pair.second.count()))
                        {
                            pair = get_pair();
                            return base_type::is_empty(pair.first.count(), pair.second.count());
                        }
                        return false;
                    }

                public:
                    using outer_allocator_value_type = T;
                    using outer_allocator_type = allocator_type::outer_allocator_type;
                    using inner_allocator_value_type = ring_detail::state_composition;
                    using inner_allocator_type = allocator_type::inner_allocator_type::outer_allocator_type;

                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        : m_compair(splits::one_v, alloc)
                    {
                        composition_t& comp = get_composition();
                        comp.m_state = std::allocator_traits<inner_allocator_type>::allocate(get_inner_allocator(), base_type::capacity());
                        comp.m_data = std::allocator_traits<outer_allocator_type>::allocate(get_outer_allocator(), base_type::capacity());
                        for (size_t at{ }; at < base_type::capacity(); ++at)
                        {
                            std::allocator_traits<inner_allocator_type>::construct(get_inner_allocator(), comp.m_state.ref() + at, action_state_value_t::poped, at, at);
                            std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), comp.m_data.ref() + at);
                        }
                    }
                    constexpr ~ring()
                    {
                        composition_t& comp = get_composition();
                        std::allocator_traits<inner_allocator_type>::deallocate(get_inner_allocator(), comp.m_state.ref(), base_type::capacity());
                        std::allocator_traits<outer_allocator_type>::deallocate(get_outer_allocator(), comp.m_data.ref(), base_type::capacity());
                    }

                    outer_allocator_type& get_outer_allocator() noexcept { return m_compair.first().outer_allocator(); }
                    inner_allocator_type& get_inner_allocator() noexcept { return m_compair.first().inner_allocator().outer_allocator(); }

                    constexpr bool is_empty() noexcept { return size() == 0; }
                    constexpr bool is_full() noexcept { return size() == base_type::capacity(); }
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

                    template <typename... Tys>
                    constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {
                        composition_t& comp = get_composition();
                        for (auto pair {get_pair()}; !double_check_is_full(pair); /*null*/)
                        {
                            state_type& input_state = (comp.m_state.ref() + pair.second.offset())->m_input_state.ref();
                            if (state_enter(input_state, pair.second))
                            {
                                counter_inc(comp.m_end.ref(), comp.m_end->load(std::memory_order::relaxed));
                                action_state_type& last_action = (comp.m_state.ref() + pair.second.offset())->m_last_action_state.ref();
                                state_wait(last_action, action_state_value_t::poped);
                                construct_at(address(pair.second.offset()), std::forward<Tys>(args)...);
                                last_action.set(action_state_value_t::pushed);
                                return true;
                            }
                            else
                            { pair.second.inc(); }
                        }
                        return false;
                    }

                    template <typename Ty>
                        requires (std::is_assignable_v<Ty, T&> || std::is_assignable_v<Ty, T&&>)
                    constexpr bool try_extract_front(Ty&& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                        )
                    {
                        composition_t& comp = get_composition();
                        for (auto pair {get_pair()}; !double_check_is_empty(pair); /*null*/)
                        {
                            state_type& output_state = (comp.m_state.ref() + pair.first.offset())->m_output_state.ref();
                            if (state_enter(output_state, pair.first))
                            {
                                counter_inc(comp.m_begin.ref(), comp.m_begin->load(std::memory_order::relaxed));
                                action_state_type& last_action = (comp.m_state.ref() + pair.first.offset())->m_last_action_state.ref();
                                state_wait(last_action, action_state_value_t::pushed);
                                T* target = address(pair.first.offset());
                                if constexpr (std::is_assignable_v<Ty, T&&>) { arg = std::move(*target); }
                                else { arg = *target; }
                                destruct_at(target);
                                last_action.set(action_state_value_t::poped);
                                return true;
                            }
                            else
                            { pair.first.inc(); }
                        }
                        return false;
                    }
                };
            } // namespace mpmc
        } // namespace concurrency
    } // namespace sia