#pragma once

// #pragma comment(lib, "Kernel32")
// #include <Windows.h>

#include <thread>
#include <functional>

// #include "SIA/concurrency/internals/define.hpp"

#include "SIA/utility/tools.hpp"
#include "SIA/utility/recorder.hpp"
#include "SIA/utility/types/function_pointer.hpp"


namespace sia
{
    namespace tags
    {
        enum class wait { busy, yield, sleep_for, sleep_until };
        enum class loop { busy, repeat_n, repeat_for, repeat_until };
    } // namespace tags

    namespace stamps
    {
        namespace basis
        {
            constexpr const default_time_rep_t empty_loop_val = 0;
            constexpr const default_time_rep_t empty_wait_val = 0;
        } // namespace tools
    } // namespace stamps
    

    namespace tools_detail
    {
        template <tags::wait Tag, typename TimeType = default_time_rep_t>
        // consteval bool is_wait_nothrow(TimeType time = stamps::basis::empty_wait_val) noexcept
        consteval bool is_wait_nothrow(TimeType time = TimeType{ }) noexcept
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
    
    template <tags::wait Tag, typename WaitTimeType = default_time_rep_t>
    constexpr void wait(WaitTimeType time = stamps::basis::empty_wait_val) noexcept(tools_detail::is_wait_nothrow<Tag, WaitTimeType>())
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
        template <tags::wait WaitTag , typename FpType, typename... Ts, typename CompType, typename WaitTimeType = default_time_rep_t>
        constexpr bool busy_loop_impl(CompType&& loop_out_cond, WaitTimeType wt_v, FpType fp, Ts&&... args)
            noexcept
            (
                tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() &&
                function_info_t<FpType>::nothrow_flag
            )
        {
            while (loop_out_cond != std::invoke(fp, std::forward<Ts>(args)...))
            { wait<WaitTag>(wt_v); }
            return true;
        }

        template <tags::wait WaitTag, typename FpType, typename... Ts, typename CompType, typename WaitTimeType = default_time_rep_t>
        constexpr bool n_loop_impl(CompType&& loop_out_cond, size_t n , WaitTimeType wt_v, FpType fp, Ts&&... args)
            noexcept
            (
                tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() &&
                function_info_t<FpType>::nothrow_flag
            )
        {
            for (size_t count { }; count < n; ++count)
            {
                if (loop_out_cond == std::invoke(fp, std::forward<Ts>(args)...))
                { return true;}
                else
                { wait<WaitTag>(wt_v); }
            }
            return false;
        }

        template <tags::wait WaitTag, typename FpType, typename... Ts, typename CompType, typename Ratio, typename Rep = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
        constexpr bool for_loop_impl(CompType&& loop_out_cond, std::chrono::duration<Rep, Ratio> time, WaitTimeType wt_v, FpType fp, Ts&&... args)
            noexcept
            (
                tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() &&
                function_info_t<FpType>::nothrow_flag
            )
        {
            single_recorder sr { };
            sr.set();
            do
            {
                if (loop_out_cond == std::invoke(fp, std::forward<Ts>(args)...))
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

        template <tags::wait WaitTag, typename FpType, typename... Ts, typename CompType, typename Clock, typename Duration, typename WaitTimeType = default_time_rep_t>
        constexpr bool until_loop_impl(CompType&& loop_out_cond, std::chrono::time_point<Clock, Duration> tp, WaitTimeType wt_v, FpType fp, Ts&&... args)
            noexcept
            (
                tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() &&
                function_info_t<FpType>::nothrow_flag
            )
        {
            do
            {
                if (loop_out_cond == std::invoke(fp, std::forward<Ts>(args)...))
                { return true;}
                else
                { wait<WaitTag>(wt_v); }
            }
            while(tp < Clock::now());
            return false;
        }
    } // namespace tools_detail

    template <tags::loop LoopTag, tags::wait WaitTag, typename FpType, typename... Ts, typename CompType, typename LoopTimeType = default_time_rep_t, typename WaitTimeType = default_time_rep_t>
        requires (std::is_invocable_v<FpType, Ts...>)
    constexpr bool loop(CompType&& loop_out_cond, LoopTimeType lt_v, WaitTimeType wt_v, FpType fp, Ts&&... args)
        noexcept
        (
            tools_detail::is_wait_nothrow<WaitTag, WaitTimeType>() &&
            function_info_t<FpType>::nothrow_flag
        )
    {
        if constexpr (LoopTag == tags::loop::busy)
        { return tools_detail::busy_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), wt_v, fp, std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_n)
        { return tools_detail::n_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, fp, std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_for)
        { return tools_detail::for_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, fp, std::forward<Ts>(args)...); }
        else if constexpr (LoopTag == tags::loop::repeat_until)
        { return tools_detail::until_loop_impl<WaitTag>(std::forward<CompType>(loop_out_cond), lt_v, wt_v, fp, std::forward<Ts>(args)...); }
        else
        { return false; }
    }

    template <typename Func>
    constexpr bool while_expression_exchange_weak(Func op, auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(op(expect, desire)))
    {
        while (op(expect, desire))
        {
            if (atomic.compare_exchange_weak(expect, desire, success_order, failure_order))
            { return true; }
        }
        return false;
    }

    template <typename Func>
    constexpr bool while_expression_exchange_strong(Func op, auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(op(expect, desire)))
    {
        while (op(expect, desire))
        {
            if (atomic.compare_exchange_strong(expect, desire, success_order, failure_order))
            { return true; }
        }
        return false;
    }
    
    // exchg_weak
    constexpr bool while_equal_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_not_equal_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::not_equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::not_equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_less_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::less<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::less<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_less_equal_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::less_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::less_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_greater_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::greater<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::greater<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_greater_equal_exchange_weak(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::greater_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_weak(std::greater_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    // exchg_strong
    constexpr bool while_equal_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_not_equal_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::not_equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::not_equal_to<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_less_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::less<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::less<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_less_equal_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::less_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::less_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_greater_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::greater<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::greater<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }

    constexpr bool while_greater_equal_exchange_strong(auto&& atomic, auto&& expect, auto desire, std::memory_order success_order, std::memory_order failure_order)
        noexcept(noexcept(std::greater_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }(expect, desire)))
    { return while_expression_exchange_strong(std::greater_equal<std::remove_reference_t<decltype(atomic)>::template value_type>{ }, atomic, expect, desire, success_order, failure_order); }
} // namespace sia
