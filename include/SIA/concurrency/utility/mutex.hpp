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
            using atomic_t = std::atomic<thread_id_t>;
            atomic_t m_owner;

            thread_id_t get_thread_id() noexcept
            { return stamps::this_thread::id_v; }

        public:
            constexpr mutex() noexcept : m_owner()
            { static_assert(atomic_t::is_always_lock_free); }
            constexpr mutex(const mutex&) noexcept = delete;
            constexpr mutex& operator=(const mutex&) noexcept = delete;
            constexpr mutex(mutex&&) noexcept = delete;
            constexpr mutex& operator=(mutex&&) noexcept = delete;

            bool try_lock(thread_id_t default_arg = thread_id_t{ }) noexcept
            { return this->m_owner.compare_exchange_weak(default_arg, this->get_thread_id(), stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v); }

            template <tags::loop LoopTag, tags::wait WaitTag, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
            bool try_lock_loop(LoopTimeType ltt_v = stamps::basis::empty_loop_val, WaitTimeType wtt_v = stamps::basis::empty_wait_val) noexcept
            { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &mutex::try_lock, this, thread_id_t{ }); }

            void lock() noexcept
            { try_lock_loop<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val); }
            
            void unlock() noexcept
            {
                if (this->m_owner.load(stamps::memory_orders::relaxed_v) == this->get_thread_id())
                { this->m_owner.store(thread_id_t{ }, stamps::memory_orders::relaxed_v); }
            }

            thread_id_t owner() noexcept { return m_owner.load(stamps::memory_orders::relaxed_v); }

            void force_lock() noexcept
            { this->m_owner.store(this->get_thread_id(), stamps::memory_orders::relaxed_v); }

            void force_unlock() noexcept
            { this->m_owner.store(thread_id_t{ }, stamps::memory_orders::relaxed_v); }
    };
} // namespace sia
