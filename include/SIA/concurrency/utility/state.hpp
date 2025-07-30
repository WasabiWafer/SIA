#pragma once

#include <atomic>

#include "SIA/concurrency/internals/define.hpp"
#include "SIA/concurrency/utility/tools.hpp"


namespace sia
{
    template <typename T>
        requires (std::atomic<T>::is_always_lock_free)
    struct state
    {
        private:
            using atomic_t = std::atomic<T>;
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

            constexpr T status() const noexcept(noexcept(m_state.load(std::memory_order::seq_cst)))
            { return m_state.load(std::memory_order::seq_cst); }

            constexpr void set(T arg) noexcept(noexcept(m_state.store(arg, std::memory_order::seq_cst)))
            { m_state.store(arg, std::memory_order::seq_cst); }

            constexpr bool compare_exchange_weak(T& expt, T desr) noexcept(noexcept(m_state.compare_exchange_weak(expt, desr, std::memory_order::seq_cst)))
            { return m_state.compare_exchange_weak(expt, desr, std::memory_order::seq_cst); }

            constexpr bool compare_exchange_strong(T& expt, T desr) noexcept(noexcept(m_state.compare_exchange_strong(expt, desr, std::memory_order::seq_cst)))
            { return m_state.compare_exchange_strong(expt, desr, std::memory_order::seq_cst); }
    };
} // namespace sia
