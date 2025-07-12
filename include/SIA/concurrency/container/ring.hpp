#pragma once

#include <memory>
#include <scoped_allocator>
#include <atomic>

#include "SIA/utility/compressed_pair.hpp"
#include "SIA/concurrency/utility/state.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/concurrency/internals/types.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace spsc
        {
            namespace ring_detail
            {
                template <typename T, T Size>
                    requires(std::atomic<T>::is_always_lock_free)
                struct ring_counter
                {
                    using counter_type = T;
                    using atomic_t = std::atomic<counter_type>;
                    atomic_t m_counter;

                    static constexpr counter_type adjustment() noexcept
                    { return std::numeric_limits<counter_type>::max() % Size; }

                    constexpr void inc() noexcept
                    {
                        constexpr auto rlx = stamps::memory_orders::relaxed_v;
                        if (m_counter.load(rlx) == std::numeric_limits<counter_type>::max())
                        { m_counter.store(adjustment() + 1); }
                        else
                        { m_counter.fetch_add(1); }
                    }

                    constexpr void dec() noexcept
                    {
                        constexpr auto rlx = stamps::memory_orders::relaxed_v;
                        if (m_counter.load(rlx) == std::numeric_limits<counter_type>::min())
                        { m_counter.store(std::numeric_limits<counter_type>::max() - adjustment() - 1); }
                        else
                        { m_counter.fetch_sub(1); }
                    }
                    
                    constexpr void add(const counter_type& arg) noexcept { m_counter.fetch_add(arg); }
                    constexpr void sub(const counter_type& arg) noexcept { m_counter.fetch_sub(arg); }
                    constexpr counter_type count() const noexcept { return m_counter.load(stamps::memory_orders::relaxed_v); }
                    constexpr counter_type offset() const noexcept { return count() % Size; }
                };

                enum class direction { none, front, back };
                enum class ring_input_state { none, constructing, constructed, placing, placed, end };
                enum class ring_output_state { none, constructing, constructed, placing, placed };

                template <typename T, size_t Size>
                struct ring_composition
                {
                    // using state_type = ring_state;
                    false_share<ring_counter<size_t, Size>> m_begin;
                    false_share<ring_counter<size_t, Size>> m_end;
                    false_share<state<ring_input_state>> m_in_state;
                    false_share<state<ring_output_state>> m_out_state;
                    false_share<T**> m_data;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::scoped_allocator_adaptor<std::allocator<T>, std::allocator<T*>>>
            struct ring
            {
                private:
                    using in_state_t = ring_detail::ring_input_state;
                    using out_state_t = ring_detail::ring_output_state;
                    using ring_counter_t = ring_detail::ring_counter<size_t, Size>;
                    using allocator_type = Allocator;
                    using composition_t = ring_detail::ring_composition<T, Size>;

                    compressed_pair<allocator_type, composition_t> m_compair;
                
                    constexpr composition_t& get_composition() noexcept { return m_compair.second(); }

                    constexpr T* address(size_t at) noexcept { return get_composition().m_data->ptr() + at;}

                    template <typename... Tys>
                    constexpr void construct_at(T* at, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { std::allocator_traits<allocator_type>::construct(get_outer_allocator(), at, std::forward<Tys>(args)...); }

                    constexpr void destruct_at(T* at) noexcept(std::is_nothrow_destructible_v<T>)
                    { std::allocator_traits<allocator_type>::destroy(get_outer_allocator(), at); }

                    constexpr void input_error_handle() noexcept
                    {
                        composition_t& comp = get_composition();
                        in_state_t state = comp.m_in_state->status();
                        if ((state == in_state_t::none) || (state == in_state_t::end))
                        { }
                        else if (state == in_state_t::constructing)
                        { }
                        else if (state == in_state_t::constructed)
                        { }
                        else if (state == in_state_t::placing)
                        { }
                        else if (state == in_state_t::placed)
                        { }
                    }

                public:
                    constexpr ring(const allocator_type& alloc = allocator_type{ })
                        noexcept(std::is_nothrow_constructible_v<allocator_type, const allocator_type&>)
                        : m_compair(splits::one_v, alloc)
                    { get_composition().m_data = std::allocator_traits<allocator_type>::allocate(get_inner_allocator(), capacity()); }

                    template <typename... Tys>
                    constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    {
                        if (is_full())
                        { return false; }
                        else
                        {
                            composition_t& comp = get_composition();
                            comp.m_in_state->set(in_state_t::constructing);
                            comp.m_in_state->set(in_state_t::constructed);
                            comp.m_in_state->set(in_state_t::placing);
                            comp.m_in_state->set(in_state_t::placed);
                        }
                        return false;
                    }

                    constexpr allocator_type& get_inner_allocator() noexcept { return m_compair.first().inner_allocator(); }
                    constexpr allocator_type& get_outer_allocator() noexcept { return m_compair.first().outer_allocator(); }
                    constexpr size_t capacity() noexcept { return Size; }
                    constexpr size_t size() noexcept
                    {
                        constexpr const size_t adj = ring_counter_t::adjustment();
                        composition_t& comp = get_composition();
                        size_t beg_count = comp.m_begin->count();
                        size_t end_count = comp.m_end->count();
                        if (end_count >= beg_count)
                        { return end_count - beg_count; }
                        else
                        { return (end_count - beg_count) - (adj + 1); }
                    }
                    constexpr bool is_empty() noexcept { return size() == 0; }
                    constexpr bool is_full() noexcept { return size() == capacity(); }
            };
        } // namespace spsc
        
        namespace mpmc
        {
            namespace ring_detail
            {
                
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::allocator<T>>
            struct ring
            {

            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia