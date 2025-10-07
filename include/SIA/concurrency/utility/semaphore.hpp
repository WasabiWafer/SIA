#pragma once

#include <limits>
#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/concurrency/utility/tools.hpp"
#include "SIA/utility/recorder.hpp"

namespace sia
{
    template <typename ValueType = largest_unsigned_integer_t, ValueType Limit = std::numeric_limits<ValueType>::max()>
    struct semaphore
    {
        private:
            using value_type = ValueType;
            using atomic_type = std::atomic<value_type>;

            atomic_type m_count;

            static constexpr value_type num_step() noexcept { return value_type(1); }

        public:
            constexpr semaphore(value_type init = Limit) noexcept
                : m_count(init)
            { static_assert(atomic_type::is_always_lock_free); }

            semaphore(const semaphore&) = delete;
            semaphore(semaphore&&) = delete;
            semaphore& operator=(const semaphore&) = delete
            semaphore& operator=(semaphore&&) = delete;

            constexpr bool try_acquire() noexcept
            {
                constexpr auto acq = std::memory_order::acquire;
                constexpr auto rle = std::memory_order::release;

                value_type cur = this->m_count.load(acq);
                if (cur != 0)
                {
                    while (!this->m_count.compare_exchange_weak(cur, cur - this->num_step(), rle, acq))
                    {
                        if (cur == 0)
                        { return false; }
                    }
                    return true;
                }
                else
                { return false; }
            }

            template <tags::loop LoopTag = tags::loop::busy, tags::wait WaitTag = tags::wait::busy, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
            constexpr bool try_acquire_loop(LoopTimeType ltt_v = stamps::basis::empty_loop_val, WaitTimeType wtt_v = stamps::basis::empty_wait_val) noexcept
            { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, static_cast<bool(semaphore::*)()>(&semaphore::try_acquire), this); }

            constexpr void acquire() noexcept
            { try_acquire_loop<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val); }

            constexpr void release() noexcept
            { this->m_count.fetch_add(this->num_step(), std::memory_order::relaxed); }
    };
} // namespace sia
