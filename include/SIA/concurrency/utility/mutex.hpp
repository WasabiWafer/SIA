#pragma once

#include <thread>
#include <atomic>

#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/concurrency/utility/tools.hpp"
#include "SIA/utility/recorder.hpp"

namespace sia
{    
    struct mutex
    {
    private:
        std::atomic<thread_id_t> m_owner;

        thread_id_t get_thread_id() noexcept
        { return sia::stamps::this_thread::id_v; }

    public:
        constexpr mutex() noexcept : m_owner()
        { assertm(m_owner.is_always_lock_free, ""); }

        template <tags::wait Tag = tags::wait::busy>
        void lock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            thread_id_t default_thread_id_v { };
            while(!this->m_owner.compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order))
            { wait<Tag>(); }
        }

        void lock(const tags::wait& tag) noexcept
        {
            if (tag == tags::wait::busy)
            { this->lock<tags::wait::busy>(); }
            else if (tag == tags::wait::yield)
            { this->lock<tags::wait::yield>(); }
        }

        bool try_lock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t default_thread_id_v { };
            return this->m_owner.compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
        }

        template <tags::time_unit Unit = tags::time_unit::seconds, tags::wait Tag = tags::wait::busy>
        bool try_lock(float time) noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            single_recorder sr{ };
            thread_id_t default_thread_id_v { };
            bool ret { };

            sr.set();
            ret = this->m_owner.compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
            sr.now();
            while (!ret && (sr.reuslt<Unit, float>() < time))
            {
                wait<Tag>();
                ret = this->m_owner.compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
                sr.now();
            }
            return ret;
        }

        template <tags::time_unit Unit = tags::time_unit::seconds>
        bool try_lock(float time, tags::wait tag) noexcept
        {
            if (tag == tags::wait::busy)
            { return this->try_lock<Unit, tags::wait::busy>(time); }
            else if (tag == tags::wait::yield)
            { return this->try_lock<Unit, tags::wait::yield>(time); }
            return false;
        }
        
        bool unlock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t default_thread_id_v { };
            thread_id_t this_thread_id_v { this->get_thread_id() };
            return this->m_owner.compare_exchange_weak(this_thread_id_v, default_thread_id_v, mem_order);
        }
    };
} // namespace sia
