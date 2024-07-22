#pragma once

#include <atomic>

#include "SIA/internals/types.hpp"
#include "SIA/utility/align_wrapper.hpp"

namespace sia
{    
    struct conditional_variable
    {
    private:
        false_share<std::atomic_flag> flag;
    public:
        template <typename Mtx_t, typename F, typename... Ts>
        constexpr void wait(Mtx_t& mtx, F&& func, Ts&&... args) noexcept
        {
            mtx.unlock();
            while(!std::forward<F>(func)(std::forward<Ts>(args)...)) { flag->wait(false, std::memory_order_acquire); }
            mtx.lock();
        }

        void notify_one() noexcept { flag->notify_one(); }
        void notify_all() noexcept { flag->notify_all(); }
    };
} // namespace sia
