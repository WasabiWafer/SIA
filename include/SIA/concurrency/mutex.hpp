#pragma once

#include <thread>
#include <atomic>

#include "SIA/internals/tags.hpp"
#include "SIA/internals/types.hpp"
#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/timer.hpp"

namespace sia
{
    struct mutex
    {
        private:
        false_share<std::atomic_flag> atf;
        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;
        public:
        constexpr mutex() noexcept = default;
        void unlock() noexcept { atf->clear(std::memory_order_relaxed); }
        void lock() noexcept
        {
            while (atf->test_and_set(std::memory_order_acquire))
            {
                while (atf->test(std::memory_order_relaxed))
                {
                    std::this_thread::yield();
                }
            }
        }
        bool try_lock() noexcept { return !atf->test_and_set(std::memory_order_acquire); }
        template <typename Contain_t, auto E0, auto E1>
        bool try_lock(std::chrono::duration<Contain_t, std::ratio<E0, E1>> time) noexcept
        {
            single_timer timer{ };
            while (atf->test_and_set(std::memory_order_acquire))
            {
                while (atf->test(std::memory_order_relaxed))
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
