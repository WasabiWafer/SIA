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
        false_share<std::atomic<value_t>> m_count;
        false_share<std::atomic<value_t>> m_core;

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

        template <tags::wait Tag>
        constexpr bool proc_loop_compare(value_t& arg) noexcept
        {
            if (Limit == std::numeric_limits<value_t>::max())
            { return arg == this->count_limit(); }
            else
            { return arg >= this->count_limit(); }
        }

    public:
        constexpr semaphore(largest_unsigned_integer_t init = 0) noexcept
            : m_count(init), m_core(init)
        { assertm(this->m_count->is_always_lock_free, ""); }

        template <tags::wait Tag = tags::wait::busy>
        constexpr void acquire() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            value_t result { };
            loop_label:
            while (this->proc_loop_compare<Tag>(result = this->m_count->fetch_add(this->num_step(), mem_order)))
            {
                this->m_count->fetch_sub(this->num_step(), mem_order);
                this->proc_tag<Tag>();
            }
            if (this->m_core->load(mem_order) <= result)
            {
                while (this->m_core->compare_exchange_weak(result, result + 1, mem_order))
                { this->proc_tag<Tag>(); }
            }
            else
            {
                result = this->m_count->fetch_sub(this->num_step(), mem_order);
                goto loop_label;
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
        constexpr void try_acqure() noexcept
        {
            value_t result { };
            
        }

        constexpr void release() noexcept
        {
            constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
            this->m_core->fetch_sub(this->num_step(), mem_order);
            this->m_count->fetch_sub(this->num_step(), mem_order);
        }
    };
} // namespace sia
