#pragma once

#include <thread>
#include <atomic>

#include "SIA/concurrency/internals/define.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/recorder.hpp"

namespace sia
{    
    struct mutex
    {
    private:
        using thread_id_t = decltype(std::declval<std::thread::id>()._Get_underlying_id());
        false_share<std::atomic<thread_id_t>> m_owner;

        thread_id_t get_thread_id() noexcept(noexcept(std::this_thread::get_id()))
        { return sia::stamps::this_thread::id_v; }
    public:
        constexpr mutex() noexcept : m_owner()
        { assertm(m_owner->is_always_lock_free, ""); }

        template <tags::wait Tag = tags::wait::busy>
        void lock() noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t default_thread_id_v { };
            if (Tag == tags::wait::busy)
            {
                while(!this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order))
                { }
            }
            else if (Tag == tags::wait::yield)
            {
                while(!this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order))
                { std::this_thread::yield(); }
            }
        }

        void lock(const tags::wait& tag) noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t default_thread_id_v { };
            if (tag == tags::wait::busy)
            {
                while(!this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order))
                { }
            }
            else if (tag == tags::wait::yield)
            {
                while(!this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order))
                { std::this_thread::yield(); }
            }
        }

        template <tags::wait Tag = tags::wait::busy>
        bool try_lock() noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t default_thread_id_v { };
            return this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
        }

        template <tags::time_unit Unit = tags::time_unit::seconds, tags::wait Tag = tags::wait::busy>
        bool try_lock(float time) noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            single_recorder st{ };
            thread_id_t default_thread_id_v { };
            bool ret { };
            st.set();
            ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
            st.now();
            if (Tag == tags::wait::busy)
            {
                while (!ret && (0.f < (time - st.reuslt<Unit>().count())))
                {
                    ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
                    st.now();
                }
            }
            else if (Tag == tags::wait::yield)
            {
                while (!ret && (0.f < (time - st.reuslt<Unit>().count())))
                {
                    std::this_thread::yield();
                    ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
                    st.now();
                }
            }
            return ret;
        }

        template <tags::time_unit Unit = tags::time_unit::seconds>
        bool try_lock(float time, tags::wait tag) noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            single_recorder st{ };
            thread_id_t default_thread_id_v { };
            bool ret { };
            st.set();
            ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
            st.now();
            if (tag == tags::wait::busy)
            {
                while (!ret && (0.f < (time - st.reuslt<Unit>().count())))
                {
                    ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
                    st.now();
                }
            }
            else if (tag == tags::wait::yield)
            {
                while (!ret && (0.f < (time - st.reuslt<Unit>().count())))
                {
                    std::this_thread::yield();
                    ret = this->m_owner->compare_exchange_weak(default_thread_id_v, this->get_thread_id(), mem_order);
                    st.now();
                }
            }
            return ret;
        }
        
        bool unlock() noexcept(noexcept(this->get_thread_id()))
        {
            constexpr auto mem_order = stamps::memory_orders::relaxed_v;
            thread_id_t this_thread_id_v { this->get_thread_id() };
            return this->m_owner->compare_exchange_weak(this_thread_id_v, 0, mem_order);
        }
    };
} // namespace sia
