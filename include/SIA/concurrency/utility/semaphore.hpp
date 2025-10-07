#pragma once

#include <limits>
#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/utility/tools.hpp"

namespace sia
{
    template <typename T = largest_unsigned_integer_t, T Limit = std::numeric_limits<T>::max()>
        requires (std::atomic<T>::is_always_lock_free)
    struct semaphore
    {
        private:
            using value_type = T;
            using atomic_type = std::atomic<value_type>;
            
            atomic_type m_count;

            static constexpr value_type step() noexcept { return value_type{1}; }
        public:
            constexpr semaphore(value_type init = Limit) noexcept
                : m_count(init)
            { }

            semaphore(const semaphore&) = delete;
            semaphore(semaphore&&) = delete;
            semaphore& operator=(const semaphore&) = delete;
            semaphore& operator=(semaphore&&) = delete;

            constexpr bool try_acquire(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept
            {
                value_type tmp = m_count.load(std::memory_order::relaxed);
                while (tmp != value_type{0})
                {
                    if (m_count.compare_exchange_weak(tmp, tmp - step(), rmw_mem_order, load_mem_order))
                    { return true; }
                }
                return false;
            }

            template <tags::loop LoopTag, tags::wait WaitTag, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
            constexpr bool try_acquire_loop(LoopTimeType ltt_v, WaitTimeType wtt_v, std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept
            { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &semaphore::try_acquire, this, rmw_mem_order, load_mem_order); }

            constexpr void acquire(std::memory_order rmw_mem_order, std::memory_order load_mem_order) noexcept
            { try_acquire_loop<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, rmw_mem_order, load_mem_order); }

            constexpr value_type release(std::memory_order mem_order) noexcept
            { return m_count.fetch_add(step(), mem_order); }
    };
} // namespace sia
