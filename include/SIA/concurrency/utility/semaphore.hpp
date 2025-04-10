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
        using self_t = semaphore;
        using value_t = ValueType;
        using atomic_t = std::atomic<value_t>;

        atomic_t m_count;

        constexpr value_t num_step(this auto&& self) noexcept { return value_t(1); }

    public:
        constexpr semaphore(value_t init = Limit) noexcept
            : m_count(init)
        { assertm(this->m_count.is_always_lock_free, ""); }

        constexpr bool try_acquire() noexcept
        {
            constexpr auto acq = stamps::memory_orders::acquire_v;
            constexpr auto rle = stamps::memory_orders::release_v;
            value_t cur = this->m_count.load(acq);
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

        template <tags::loop LoopTag = tags::loop::busy, tags::wait WaitTag = tags::wait::busy, typename LoopTimeType = default_rep_t, typename WaitTimeType = default_rep_t>
        constexpr bool try_acquire_loop(LoopTimeType ltt_v = stamps::tools::empty_loop_val, WaitTimeType wtt_v = stamps::tools::empty_wait_val) noexcept
        { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, static_cast<bool(self_t::*)()>(&self_t::try_acquire), this); }

        template <tags::wait WaitTag = tags::wait::busy, typename WaitTimeType = default_rep_t>
        constexpr void acquire(WaitTimeType wtt_v = stamps::tools::empty_wait_val) noexcept
        { try_acquire_loop<tags::loop::busy, WaitTag>(stamps::tools::empty_loop_val, wtt_v); }

        constexpr void release() noexcept
        { this->m_count.fetch_add(this->num_step(), stamps::memory_orders::relaxed_v); }
    };
} // namespace sia
