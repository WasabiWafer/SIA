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

        template <tags::wait Tag = tags::wait::busy, tags::time_unit Unit = tags::time_unit::seconds, typename Rep0 = float, typename Rep1 = float>
        constexpr bool try_acquire(Rep0 try_time, Rep1 wait_time = Rep1()) noexcept
        {
            single_recorder sr{ };
            sr.set();
            do
            {
                if (this->try_acquire())
                { return true; }
                else
                {
                    wait<Tag>(wait_time);
                    sr.now();
                }
            }
            while(sr.result<Unit, Rep0>() < try_time);
            return false;
        }

        template <tags::wait Tag = tags::wait::busy, typename Rep = float>
        constexpr void acquire(Rep wait_time = Rep()) noexcept
        {
            while(!this->try_acquire())
            { wait<Tag>(wait_time); }
        }

        constexpr void release() noexcept
        {
            constexpr auto rlx = stamps::memory_orders::relaxed_v;
            this->m_count.fetch_add(this->num_step(), rlx);
        }
    };
} // namespace sia
