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
        
    } // namespace tools_detail
    
    template <tags::wait Tag>
    constexpr void wait() noexcept
    {
        if constexpr (Tag == tags::wait::busy)
        { }
        else if constexpr (Tag == tags::wait::yield)
        { std::this_thread::yield(); }
        else
        { }
    }

    constexpr void wait(tags::wait tag) noexcept
    {
        if (tag == tags::wait::busy)
        { wait<tags::wait::busy>(); }
        else if (tag == tags::wait::yield)
        { wait<tags::wait::yield>(); }
        else
        { }
    }

    template <bool OutCond, tags::wait Tag = tags::wait::busy, typename Func = void, typename... Ts>
        requires (std::is_invocable_r_v<bool, Func, Ts...>)
    constexpr bool loop(Func&& func, Ts&&... args) noexcept(noexcept(std::invoke_r<bool>(std::forward<Func>(func), std::forward<Ts>(args)...)))
    {
        while(OutCond != std::invoke_r<bool>(std::forward<Func>(func), std::forward<Ts>(args)...))
        { wait<Tag>(); }
        return true;
    }

    template <bool OutCond, tags::wait Tag = tags::wait::busy, tags::time_unit Unit = tags::time_unit::seconds, typename Rep = float, typename Func = void, typename... Ts>
        requires (std::is_invocable_r_v<bool, Func, Ts...>)
    constexpr bool loop(Rep time, Func&& func, Ts&&... args) noexcept(noexcept(std::invoke_r<bool>(std::forward<Func>(func), std::forward<Ts>(args)...)))
    {
        single_recorder sr { };
        sr.set();
        auto cond = std::invoke_r<bool>(std::forward<Func>(func), std::forward<Ts>(args)...);
        sr.now();
        while((OutCond != cond) && (sr.reuslt<Unit, Rep>() < time))
        {
            wait<Tag>();
            cond = std::invoke_r<bool>(std::forward<Func>(func), std::forward<Ts>(args)...);
            sr.now();
        }
        return cond;
    }
} // namespace sia
