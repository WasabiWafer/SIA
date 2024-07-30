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

        single_timer() : init(std::chrono::high_resolution_clock::now()), point(init) { }

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

        template <typename F>
        constexpr void measurement(F&& func) noexcept
        {
            tm.now();
            for(size_t count {0}; count < RunCount; ++count) { func(); }
            tm.now();
        }

        template <typename Duration_t = std::chrono::nanoseconds>
        constexpr void result() noexcept
        {
            for (size_t count{0}; count < (tm.point.size() / 2); ++count)
            {
                if (count == 0)
                {
                    std::print("Function {} : {}\n", count, std::chrono::duration_cast<Duration_t>(tm[count + 1] - tm[count]));
                }
                else
                {
                    auto before = std::chrono::duration_cast<Duration_t>(tm[(count * 2) - 1] - tm[(count * 2) - 2]);
                    auto cur = std::chrono::duration_cast<Duration_t>(tm[(count * 2) + 1] - tm[(count * 2)]);
                    double percent = (1.0l - static_cast<double>(cur.count())/static_cast<double>(before.count()))*100.0l;
                    std::print("Function {} : {} ({:.3f} %)\n", count, cur, percent);
                }
            }
        }

        template <typename... Fs>
        constexpr runner(Fs&&... args) : tm() { (measurement(std::forward<Fs>(args)), ...); }
    };
} // namespace sia
