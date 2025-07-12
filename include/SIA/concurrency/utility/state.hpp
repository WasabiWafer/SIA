#pragma once

#include <atomic>

#include "SIA/concurrency/internals/define.hpp"
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
            constexpr state() noexcept(std::is_nothrow_default_constructible_v<atomic_t>)
                : m_state()
            { }

            constexpr state(T arg) noexcept(std::is_nothrow_constructible_v<atomic_t, T>)
                : m_state(arg)
            { }

            constexpr state(const state& arg) noexcept(std::is_nothrow_constructible_v<atomic_t, T>)
                : m_state(arg.status())
            { }

            constexpr state& operator=(T arg) noexcept(noexcept(this->set(arg)))
            {
                this->set(arg);
                return *this;
            }

            constexpr state& operator=(const state& arg) noexcept(noexcept(this->set(arg.status())))
            {
                this->set(arg.status());
                return *this;
            }

            constexpr T status() const noexcept(noexcept(m_state.load(stamps::memory_orders::relaxed_v)))
            { return m_state.load(stamps::memory_orders::relaxed_v); }

            constexpr void set(T arg) noexcept(noexcept(m_state.store(arg, stamps::memory_orders::relaxed_v)))
            { m_state.store(arg, stamps::memory_orders::relaxed_v); }

            constexpr bool compare_exchange(T& expt, T desr) noexcept(m_state.compare_exchange_weak(expt, desr, stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v))
            { return m_state.compare_exchange_weak(expt, desr, stamps::memory_orders::relaxed_v, stamps::memory_orders::relaxed_v); }
    };
} // namespace sia
