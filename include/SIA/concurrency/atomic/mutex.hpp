#pragma once

#include <thread>
#include <atomic>

#include "SIA/internals/tags.hpp"
#include "SIA/internals/types.hpp"
#include "SIA/utility/timer.hpp"

namespace sia
{
    struct mutex
    {
        private:
        alignas(std::hardware_destructive_interference_size) std::atomic_flag atf;
        unsigned_interger_t<1> padding[std::hardware_destructive_interference_size - sizeof(std::atomic_flag)];
        

        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;

        public:
        constexpr mutex() noexcept = default;

        void unlock() noexcept { atf.clear(std::memory_order_relaxed); }
        void lock() noexcept
        {
            while (atf.test_and_set(std::memory_order_acquire))
            {
                while (atf.test(std::memory_order_relaxed))
                {
                    std::this_thread::yield();
                }
            }
        }

        template <typename Contain_t, auto E0, auto E1>
        bool try_lock(std::chrono::duration<Contain_t, std::ratio<E0, E1>> time) noexcept
        {
            single_timer timer{ };
            while (atf.test_and_set(std::memory_order_acquire))
            {
                while (atf.test(std::memory_order_relaxed))
                {
                    timer.now();
                    if (timer.get() <= time) { std::this_thread::yield(); }
                    else { return false; }
                }
            }
            return true;
        }

        bool try_lock(const size_t ms = 0) noexcept
        {
            single_timer timer{ };
            const auto time = std::chrono_literals::operator""ms(ms);
            while (atf.test_and_set(std::memory_order_acquire))
            {
                while (atf.test(std::memory_order_relaxed))
                {
                    timer.now();
                    if (timer.get() <= time) { std::this_thread::yield(); }
                    else { return false; }
                }
            }
            return true;
        }
    };
} // namespace sia

// test-and-set
// bool test_and_set(bool& arg)
// {
//     bool prev = arg;
//     arg = true;
//     return prev;
// }
