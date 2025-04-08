#pragma once

// #pragma comment(lib, "Kernel32")
// #include <Windows.h>

#include <functional>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/internals/define.hpp"

#include "SIA/utility/recorder.hpp"

namespace sia
{
    namespace tools_detail
    {
        template <tags::wait Tag, typename TimeType = long long>
        consteval bool is_wait_nothrow(TimeType time = TimeType()) noexcept
        {
            if constexpr (Tag == tags::wait::busy)
            { return true; }
            else if constexpr (Tag == tags::wait::yield)
            { return noexcept(std::this_thread::yield()); }
            else if constexpr (Tag == tags::wait::sleep_for)
            { return noexcept(std::this_thread::sleep_for(time)); }
            else if constexpr (Tag == tags::wait::sleep_until)
            { return noexcept(std::this_thread::sleep_until(time)); }
            else
            { return false; }
        }
    } // namespace tools_detial
    
    template <tags::wait Tag, typename TimeType = long long>
    constexpr void wait(TimeType time = TimeType()) noexcept(tools_detail::is_wait_nothrow<Tag, TimeType>(TimeType()))
    {
        if constexpr (Tag == tags::wait::busy)
        { }
        else if constexpr (Tag == tags::wait::yield)
        { std::this_thread::yield(); }
        else if constexpr (Tag == tags::wait::sleep_for)
        { std::this_thread::sleep_for(time); }
        else if constexpr (Tag == tags::wait::sleep_until)
        { std::this_thread::sleep_until(time); }
        else
        { }
    }

    namespace tools_detail
    {
        template <tags::wait WaitTag , typename Func, typename... Ts, typename CompType, typename WaitTimeType = long long>
        constexpr bool busy_loop_impl(CompType&& loop_out_cond, Func&& func, Ts&&... args, WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        {
            while (loop_out_cond != std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...))
            { wait<WaitTag>(wt_v); }
            return true;
        }

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename WaitTimeType = long long>
        constexpr bool n_loop_impl(CompType&& loop_out_cond, Func&& func, Ts&&... args, size_t n , WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        {
            for (size_t count { }; count < n; ++count)
            {
                if (loop_out_cond == std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...))
                { return true;}
                else
                { wait<WaitTag>(wt_v); }
            }
            return false;
        }

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename Ratio, typename Rep = long long, typename WaitTimeType = long long>
        // constexpr bool for_loop_impl(CompType&& loop_out_cond, Func&& func, Ts&&... args, time_exp_t<Unit, Rep> time, WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        constexpr bool for_loop_impl(CompType&& loop_out_cond, Func&& func, Ts&&... args, std::chrono::duration<Rep, Ratio> time, WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        {
            single_recorder sr { };
            sr.set();
            do
            {
                if (loop_out_cond == std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...))
                { return true;}
                else
                {
                    wait<WaitTag>(wt_v);
                    sr.now();
                }
            }
            while(sr.result() < time);
            return false;
        }

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename Clock, typename Duration, typename WaitTimeType = long long>
        constexpr bool until_loop_impl(CompType&& loop_out_cond, Func&& func, Ts&&... args, std::chrono::time_point<Clock, Duration> tp, WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        {
            do
            {
                if (loop_out_cond == std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...))
                { return true;}
                else
                { wait<WaitTag>(wt_v); }
            }
            while(tp < Clock::now());
            return false;
        }
    } // namespace tools_detail

    template <tags::loop LoopTag, tags::wait WaitTag = tags::wait::busy, typename Func, typename... Ts, typename CompType, typename LoopTimeType = long long, typename WaitTimeType = long long>
        requires (std::is_invocable_v<Func, Ts...> && (LoopTag != tags::loop::busy))
    constexpr bool loop(CompType&& loop_out_cond, Func&& func, Ts&&... args, LoopTimeType lt_v = LoopTimeType(), WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
    {
        if constexpr (LoopTag == tags::loop::busy)
        { return tools_detail::busy_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), std::forward<Func>(func), std::forward<Ts>(args)..., wt_v); }
        else if constexpr (LoopTag == tags::loop::repeat_n)
        { return tools_detail::n_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), std::forward<Func>(func), std::forward<Ts>(args)..., lt_v, wt_v); }
        else if constexpr (LoopTag == tags::loop::repeat_for)
        { return tools_detail::for_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), std::forward<Func>(func), std::forward<Ts>(args)..., lt_v, wt_v); }
        else if constexpr (LoopTag == tags::loop::repeat_until)
        { return tools_detail::until_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), std::forward<Func>(func), std::forward<Ts>(args)..., lt_v, wt_v); }
        else
        { return false; }
    }

    template <tags::loop LoopTag = tags::loop::busy, tags::wait WaitTag = tags::wait::busy, typename Func, typename... Ts, typename CompType, typename WaitTimeType = long long>
        requires (std::is_invocable_v<Func, Ts...> && (LoopTag == tags::loop::busy))
    constexpr bool loop(CompType&& loop_out_cond, Func&& func, Ts&&... args, WaitTimeType wt_v = WaitTimeType()) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
    {
        if constexpr (LoopTag == tags::loop::busy)
        { return tools_detail::busy_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), std::forward<Func>(func), std::forward<Ts>(args)..., wt_v); }
        else
        { return false; }
    }
} // namespace sia
