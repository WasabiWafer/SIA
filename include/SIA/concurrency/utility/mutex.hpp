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
            constexpr mutex() noexcept : m_owner(thread_id_t{ })
            { static_assert(atomic_t::is_always_lock_free); }
            
            constexpr mutex(const mutex&) noexcept = delete;
            constexpr mutex& operator=(const mutex&) noexcept = delete;
            constexpr mutex(mutex&&) noexcept = delete;
            constexpr mutex& operator=(mutex&&) noexcept = delete;

            bool try_lock(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            {
                thread_id_t default_arg { };
                return this->m_owner.compare_exchange_strong(default_arg, this->get_thread_id(), mem_order, std::memory_order::relaxed);
            }

            template <tags::loop LoopTag, tags::wait WaitTag, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
            bool try_lock_loop(LoopTimeType ltt_v, WaitTimeType wtt_v, std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &mutex::try_lock, this, mem_order); }

            void lock(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { try_lock_loop<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, mem_order); }
            
            void unlock(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            {
                thread_id_t tid = stamps::this_thread::id_v;
                this->m_owner.compare_exchange_strong(tid, thread_id_t{ }, mem_order, std::memory_order::relaxed);
            }

            thread_id_t owner(std::memory_order mem_order = std::memory_order::seq_cst) noexcept { return m_owner.load(mem_order); }
            constexpr bool is_own(std::memory_order mem_order = std::memory_order::seq_cst) noexcept { return owner(mem_order) == stamps::this_thread::id_v; }

            void force_lock(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { this->m_owner.store(this->get_thread_id(), mem_order); }

            void force_unlock(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { this->m_owner.store(thread_id_t{ }, mem_order); }
    };
} // namespace sia
