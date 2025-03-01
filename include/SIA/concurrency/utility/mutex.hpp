#pragma once

#include <thread>
#include <atomic>

#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/utility/recorder.hpp"

namespace sia
{
    struct mutex
    {
    private:
        false_share<std::atomic<std::thread::id>> m_owner;
        false_share<std::atomic_flag> m_flag;

    public:
        constexpr mutex() noexcept : m_owner(), m_flag()
        { assertm(m_owner->is_always_lock_free, ""); }

        void lock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::seq_cst_v;
            while(this->m_flag->test_and_set(mem_order))
            { this->m_flag->wait(true, mem_order); }
            m_owner->store(std::this_thread::get_id(), mem_order);
        }

        bool try_lock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::seq_cst_v;
            if (!m_flag->test_and_set(mem_order))
            {
                m_owner->store(std::this_thread::get_id(), mem_order);
                return true;
            }
            return false;
        }

        template <tags::time_unit Unit = tags::time_unit::seconds>
        bool try_lock(float time) noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::seq_cst_v;
            single_recorder st{ };
            bool ret { };
            st.set();
            do
            {
                ret = !m_flag->test_and_set(mem_order);
                st.now();
            }
            while ( ret && (0.f < (time - st.reuslt<Unit>().count())) );
            return !ret;
        }
        
        void unlock() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::seq_cst_v;
            if (std::this_thread::get_id() == m_owner->load(mem_order))
            {
                m_flag->clear(mem_order);
                m_flag->notify_one();
            }
        }
    };
} // namespace sia
