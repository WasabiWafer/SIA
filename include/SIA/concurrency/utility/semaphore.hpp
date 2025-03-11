#pragma once

#include <limits>
#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/internals/define.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/recorder.hpp"

namespace sia
{
    template <typename ValueType = largest_unsigned_integer_t, ValueType Limit = std::numeric_limits<ValueType>::max()>
    struct semaphore
    {
    private:
        using value_t = ValueType;
        using atomic_t = std::atomic<value_t>;
        using wrap_t = false_share<atomic_t>;
        wrap_t m_count;
        

        constexpr value_t count_limit(this auto&& self) noexcept { return Limit;}
        constexpr value_t num_step(this auto&& self) noexcept { return value_t(1); }
        template <tags::wait Tag>
        constexpr void proc_tag() noexcept
        {
            if (Tag == tags::wait::busy)
            { }
            else if (Tag == tags::wait::yield)
            { std::this_thread::yield(); }
        }

    public:
        constexpr semaphore(value_t init = Limit) noexcept
            : m_count(init)
        { assertm(this->m_count->is_always_lock_free, ""); }

        template <tags::wait Tag = tags::wait::busy>
        constexpr void acquire() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            bool loop_cond { };
            value_t num {this->m_count->load(mem_order)};
            while (!loop_cond)
            {
                if (num != 0)
                {
                    loop_cond = this->m_count->compare_exchange_strong(num, num - this->num_step(), mem_order);
                    if (!loop_cond)
                    { this->proc_tag<Tag>(); }
                }
                else
                { this->proc_tag<Tag>(); }
            }
        }

        constexpr void acquire(const tags::wait& tag) noexcept
        {
            if (tag == tags::wait::busy)
            { this->acquire<tags::wait::busy>(); }
            else if (tag == tags::wait::yield)
            { this->acquire<tags::wait::yield>(); }
        }

        template <tags::wait Tag = tags::wait::busy>
        constexpr bool try_acquire() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            bool loop_cond { };
            value_t num {this->m_count->load(mem_order)};
            while ((num != 0) && (!loop_cond))
            {
                loop_cond = this->m_count->compare_exchange_strong(num, num - this->num_step(), mem_order);
                if (!loop_cond)
                { this->proc_tag<Tag>(); }
            }
            return loop_cond;
        }

        template <tags::time_unit Unit = tags::time_unit::seconds, tags::wait Tag = tags::wait::busy>
        constexpr bool try_acquire(float time) noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            single_recorder sr{ };
            bool loop_cond { };
            value_t num {this->m_count->load(mem_order)};
            if (num != 0)
            {
                sr.set();
                loop_cond = this->m_count->compare_exchange_strong(num, num - this->num_step(), mem_order);
                sr.now();
                while ((num != 0) && (!loop_cond) && (sr.reuslt<Unit, float>() < time))
                {
                    this->proc_tag<Tag>();
                    loop_cond = this->m_count->compare_exchange_strong(num, num - this->num_step(), mem_order);
                    sr.now();
                }
            }
            return loop_cond;
        }

        template <tags::time_unit Unit = tags::time_unit::seconds>
        constexpr bool try_acquire(float time, tags::wait tag) noexcept
        {
            if (tag == tags::wait::busy)
            { return this->try_acquire<Unit, tags::wait::busy>(time); }
            else if (tag == tags::wait::yield)
            { return this->try_acquire<Unit, tags::wait::yield>(time); }
            return false;
        }

        constexpr void release() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            this->m_count->fetch_add(this->num_step(), mem_order);
        }
    };
} // namespace sia
