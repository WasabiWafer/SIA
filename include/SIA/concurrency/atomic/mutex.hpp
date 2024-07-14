#pragma once

#include <thread>
#include <atomic>
#include <chrono>

namespace sia
{
    struct mutex
    {
        private:
        std::atomic_flag atf;

        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;

        public:
        constexpr mutex() noexcept = default;

        void unlock() noexcept { atf.clear(std::memory_order_release); }
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

        bool try_lock() noexcept
        {

        }
    };

    template <typename Mtx_t>
    struct lock_guard
    {
        Mtx_t& mtx;
        lock_guard(Mtx_t& arg) : mtx{arg} { mtx.lock(); }
        ~lock_guard() { mtx.unlock(); }
    };
} // namespace sia

// test-and-set
// bool test_and_set(bool& arg)
// {
//     bool prev = arg;
//     arg = true;
//     return prev;
// }
