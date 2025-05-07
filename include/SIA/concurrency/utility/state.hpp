#pragma once

#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/internals/define.hpp"

#include "SIA/concurrency/utility/tools.hpp"


namespace sia
{
    struct state
    {
    private:
        using atomic_t = volatile std::atomic<tags::object_state>;
        atomic_t m_state;
    public:
        constexpr state() noexcept = default;
        constexpr state(state& arg) noexcept
            : m_state(arg.status())
        { }
        constexpr state(const tags::object_state& arg) noexcept
            : m_state(arg)
        { }
        constexpr state& operator=(state& arg) noexcept
        {
            this->set(arg.status());
            return *this;
        }

        constexpr tags::object_state status() noexcept
        { return m_state.load(stamps::memory_orders::relaxed_v); }

        constexpr void set(tags::object_state arg) noexcept
        { m_state.store(arg, stamps::memory_orders::relaxed_v); }

        constexpr bool compare_exchange(tags::object_state& expt, tags::object_state desr) noexcept
        { return m_state.compare_exchange_weak(expt, desr, stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v); }
    };
} // namespace sia
