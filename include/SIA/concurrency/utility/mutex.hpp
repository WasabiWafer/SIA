#pragma once

#include <thread>
#include <atomic>

#include "SIA/utility/tools.hpp"
#include "SIA/utility/recorder.hpp"
#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/concurrency/utility/tools.hpp"

namespace sia
{    
    struct mutex
    {
    private:
        using self_t = mutex;
        using value_t = std::atomic<thread_id_t>;
        value_t m_owner;

        thread_id_t get_thread_id() noexcept
        { return sia::stamps::this_thread::id_v; }

    public:
        constexpr mutex() noexcept : m_owner()
        { assertm(m_owner.is_always_lock_free, ""); }

        bool try_lock() noexcept
        {
            thread_id_t default_thread_id_v { };
            return this->m_owner.compare_exchange_weak(default_thread_id_v, this->get_thread_id(), stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v);
        }

        template <tags::loop LoopTag, tags::wait WaitTag = tags::wait::busy, typename LoopTimeType = default_rep_t, typename WaitTimeType = default_rep_t>
        bool try_lock_loop(LoopTimeType ltt_v = stamps::tools::empty_loop_val, WaitTimeType wtt_v = stamps::tools::empty_wait_val) noexcept
        { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &self_t::try_lock, this); }

        template <tags::wait WaitTag = tags::wait::busy, typename WaitTimeType = default_rep_t>
        void lock(WaitTimeType wtt_v = stamps::tools::empty_wait_val) noexcept
        { loop<tags::loop::busy, WaitTag>(true, stamps::tools::empty_loop_val, wtt_v, &self_t::try_lock, this); }
        
        void unlock() noexcept
        {
            constexpr auto rlx = stamps::memory_orders::relaxed_v;
            if (this->m_owner.load(rlx) == this->get_thread_id())
            { this->m_owner.store(thread_id_t(), rlx); }
        }
    };
} // namespace sia
