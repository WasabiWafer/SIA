#pragma once

#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/internals/define.hpp"

#include "SIA/concurrency/utility/tools.hpp"


namespace sia
{
    template <typename T>
        requires (type<volatile std::atomic<T>>::is_always_lock_free)
    struct state
    {
    private:
        using atomic_t = volatile std::atomic<T>;
        atomic_t m_state;
    public:
        constexpr state() noexcept = default;
        constexpr state(const state& arg) noexcept
            : m_state(arg.status())
        { }
        constexpr state(const T& arg) noexcept
            : m_state(arg)
        { }
        constexpr state& operator=(state& arg) noexcept
        {
            this->set(arg.status());
            return *this;
        }

        T status() noexcept
        { return m_state.load(stamps::memory_orders::relaxed_v); }

        void set(const T& arg) noexcept
        { m_state.store(arg, stamps::memory_orders::relaxed_v); }

        bool compare_exchange(tags::object_state& expt, const T& desr) noexcept
        { return m_state.compare_exchange_weak(expt, desr, stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v); }
    };
} // namespace sia
