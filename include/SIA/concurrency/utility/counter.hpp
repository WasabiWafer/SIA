#pragma once

#include <functional>
#include <atomic>

#include "SIA/concurrency/utility/tools.hpp"

namespace sia
{
    namespace concurrency
    {
        template <typename T, T Step = T{1}>
            requires (std::atomic<T>::is_always_lock_free)
        struct counter
        {
            private:
                using value_type = T;
                using atomic_type = std::atomic<value_type>;

                atomic_type m_atomic;

                static constexpr value_type step() noexcept { return Step; }

            public:
                constexpr value_type count(std::memory_order mem_order) noexcept { return m_atomic.load(mem_order); }

                template <tags::wait WaitTag>
                constexpr bool try_gradual_expression_step(auto func, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                {
                    value_type tmp {count(std::memory_order::relaxed)};
                    return m_atomic.compare_exchange_strong(tmp, func(tmp, step()), rmw_mem_order, load_mem_order);
                }

                template <tags::wait WaitTag>
                constexpr void gradual_expression_step(auto func, auto wtt_v, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                {
                    value_type tmp {count(std::memory_order::relaxed)};
                    while(!m_atomic.compare_exchange_weak(tmp, func(tmp, step()), rmw_mem_order, load_mem_order))
                    { wait<WaitTag>(wtt_v); }
                }
                
                constexpr bool try_gradual_inc(std::memory_order mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { return try_gradual_expression_step(std::plus{ }, mem_order, std::memory_order::relaxed); }
                constexpr bool try_gradual_dec(std::memory_order mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { return try_gradual_expression_step(std::minus{ }, mem_order, std::memory_order::relaxed); }
                
                constexpr void gradual_inc(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { gradual_expression_step<tags::wait::busy>(std::plus{ }, stamps::basis::empty_wait_val, rmw_mem_order, load_mem_order); }
                constexpr void gradual_dec(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { gradual_expression_step<tags::wait::busy>(std::minus{ }, stamps::basis::empty_wait_val, rmw_mem_order, load_mem_order); }

                constexpr value_type inc(std::memory_order mem_order) noexcept
                { return m_atomic.fetch_add(step(), mem_order); }
                constexpr value_type dec(std::memory_order mem_order) noexcept
                { return m_atomic.fetch_sub(step(), mem_order); }
                
                constexpr value_type add(value_type amount, std::memory_order mem_order) noexcept
                { return m_atomic.fetch_add(amount, mem_order); }
                constexpr value_type sub(value_type amount, std::memory_order mem_order) noexcept
                { return m_atomic.fetch_sub(amount, mem_order); }
        };
    } // namespace concurrency
} // namespace sia
