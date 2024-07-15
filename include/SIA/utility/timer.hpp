#pragma once

#include <type_traits>
#include <print>
#include <chrono>

namespace sia
{
    struct timer
    {
        using tp_t = decltype(std::chrono::high_resolution_clock::now());

        tp_t init;
        std::vector<tp_t> point;

        timer() : init(std::chrono::high_resolution_clock::now()), point() { }

        constexpr tp_t& operator[](const size_t pos) { return point[pos]; }
        auto now() noexcept { point.emplace_back(std::chrono::high_resolution_clock::now()); }
        template <typename Duration_t = std::chrono::milliseconds>
        auto get(const size_t pos) noexcept { return std::chrono::duration_cast<Duration_t>(point[pos] - init); }
    };

    struct single_timer
    {
        using tp_t = decltype(std::chrono::high_resolution_clock::now());

        tp_t init;
        tp_t point;

        single_timer() : init(std::chrono::high_resolution_clock::now()), point() { }

        auto now() noexcept { point = std::chrono::high_resolution_clock::now(); }
        template <typename Duration_t = std::chrono::milliseconds>
        auto get() noexcept { return std::chrono::duration_cast<Duration_t>(point - init); }
    };
} // namespace sia


namespace sia
{
    template <size_t RunCount>
    struct runner
    {
        timer tm;
        size_t func_num;

        template <typename F>
        constexpr void measurement(F&& func) noexcept
        {
            tm.now();
            for(size_t count {0}; count < RunCount; ++count) { func(); }
            tm.now();
        }

        template <typename Duration_t>
        constexpr void result() noexcept
        {
            for (size_t count{0}, pos{0}; count < func_num; ++count, pos += 2)
            {
                std::print("Function {} : {}\n", count, std::chrono::duration_cast<Duration_t>(tm[pos + 1] - tm[pos]));
            }
        }

        template <typename... Fs>
        constexpr runner(Fs&&... args) : tm(), func_num(sizeof...(Fs)) { (measurement(std::forward<Fs>(args)), ...); }
    };
} // namespace sia
