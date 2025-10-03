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
                using atomic_type = std::atomic<T>;

                atomic_type m_atomic;

                static constexpr T step() noexcept { return Step; }
                constexpr bool try_expression_step(auto func, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                {
                    T tmp {count(std::memory_order::relaxed)};
                    return m_atomic.compare_exchange_strong(tmp, func(tmp, step()), rmw_mem_order, load_mem_order);
                }

                template <tags::wait WaitTag>
                constexpr void expression_step(auto func, auto wtt_v, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                {
                    T tmp {count(std::memory_order::relaxed)};
                    while(!m_atomic.compare_exchange_weak(tmp, func(tmp, step()), rmw_mem_order, load_mem_order))
                    { wait<WaitTag>(wtt_v); }
                }

            public:
                constexpr T count(std::memory_order mem_order) noexcept { return m_atomic.load(mem_order); }
                constexpr bool try_inc(std::memory_order rmw_mem_order, std::memory_order load_mem_order = std::memory_order::relaxed) noexcept(std::is_nothrow_constructible_v<T, T>)
                { return try_expression_step(std::plus{ }, rmw_mem_order, load_mem_order); }
                constexpr bool try_dec(std::memory_order rmw_mem_order, std::memory_order load_mem_order = std::memory_order::relaxed) noexcept(std::is_nothrow_constructible_v<T, T>)
                { return try_expression_step(std::minus{ }, rmw_mem_order, load_mem_order); }
                
                template <tags::wait WaitTag = tags::wait::busy>
                constexpr void inc(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { expression_step<WaitTag>(std::plus{ }, stamps::basis::empty_wait_val, rmw_mem_order, load_mem_order); }
                template <tags::wait WaitTag = tags::wait::busy>
                constexpr void dec(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { expression_step<WaitTag>(std::minus{ }, stamps::basis::empty_wait_val, rmw_mem_order, load_mem_order); }

                template <tags::wait WaitTag = tags::wait::busy>
                constexpr void inc(auto wtt_v, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { expression_step<WaitTag>(std::plus{ }, wtt_v, rmw_mem_order, load_mem_order); }
                template <tags::wait WaitTag = tags::wait::busy>
                constexpr void dec(auto wtt_v, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept(std::is_nothrow_constructible_v<T, T>)
                { expression_step<WaitTag>(std::minus{ }, wtt_v, rmw_mem_order, load_mem_order); }
        };
    } // namespace concurrency
} // namespace sia
