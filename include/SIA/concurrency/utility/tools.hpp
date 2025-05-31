#pragma once

// #pragma comment(lib, "Kernel32")
// #include <Windows.h>

#include <functional>

#include "SIA/utility/recorder.hpp"
#include "SIA/concurrency/internals/define.hpp"


namespace sia
{
    namespace stamps
    {
        namespace basis
        {
            constexpr default_time_rep_t empty_loop_val = 0;
            constexpr default_time_rep_t empty_wait_val = 0;
        } // namespace tools
    } // namespace stamps
    

    namespace tools_detail
    {
        template <tags::wait Tag, typename TimeType = default_time_rep_t>
        consteval bool is_wait_nothrow(TimeType time = stamps::basis::empty_wait_val) noexcept
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
    
    template <tags::wait Tag, typename TimeType = default_time_rep_t>
    constexpr void wait(TimeType time = stamps::basis::empty_wait_val) noexcept(tools_detail::is_wait_nothrow<Tag, TimeType>())
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
        template <tags::wait WaitTag , typename Func, typename... Ts, typename CompType, typename WaitTimeType = default_time_rep_t>
        constexpr bool busy_loop_impl(CompType&& loop_out_cond, WaitTimeType wt_v, Func&& func, Ts&&... args) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
        {
            while (loop_out_cond != std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...))
            { wait<WaitTag>(wt_v); }
            return true;
        }

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename WaitTimeType = default_time_rep_t>
        constexpr bool n_loop_impl(CompType&& loop_out_cond, size_t n , WaitTimeType wt_v, Func&& func, Ts&&... args) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
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

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename Ratio, typename Rep = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
        constexpr bool for_loop_impl(CompType&& loop_out_cond, std::chrono::duration<Rep, Ratio> time, WaitTimeType wt_v, Func&& func, Ts&&... args) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
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

        template <tags::wait WaitTag, typename Func, typename... Ts, typename CompType, typename Clock, typename Duration, typename WaitTimeType = default_time_rep_t>
        constexpr bool until_loop_impl(CompType&& loop_out_cond, std::chrono::time_point<Clock, Duration> tp, WaitTimeType wt_v, Func&& func, Ts&&... args) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
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

    template <tags::loop LoopTag, tags::wait WaitTag = tags::wait::busy, typename Func, typename... Ts, typename CompType, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
        requires (std::is_invocable_v<Func, Ts...>)
    constexpr bool loop(CompType&& loop_out_cond, LoopTimeType lt_v, WaitTimeType wt_v, Func&& func, Ts&&... args) noexcept(tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() && noexcept(std::invoke(std::forward<Func>(func), std::forward<Ts>(args)...)))
    {
        if constexpr (LoopTag == tags::loop::busy)
        { return tools_detail::busy_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), wt_v, std::forward<Func>(func), std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_n)
        { return tools_detail::n_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, std::forward<Func>(func), std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_for)
        { return tools_detail::for_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, std::forward<Func>(func), std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_until)
        { return tools_detail::until_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, std::forward<Func>(func), std::forward<Ts>(args)...); }
        else
        { return false; }
    }
} // namespace sia//
