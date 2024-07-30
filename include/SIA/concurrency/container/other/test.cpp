#include <print>
#include <ranges>
#include <chrono>
#include <vector>
#include <algorithm>

#include "SIA/utility/timer.hpp"
#include "SIA/concurrency/container/ring.hpp"
#include "SIA/concurrency/container/tail.hpp"

constexpr size_t thread_num {24};
constexpr auto time_scale = 1000;
constexpr auto input_time = std::chrono::milliseconds(time_scale);
constexpr auto output_time = std::chrono::milliseconds(time_scale*15/10);
// mpmc_bounded_queue<size_t> r0{512};
sia::concurrency::lock_free::mpmc::ring<size_t, 1024> r0{ };
std::pair<std::vector<size_t>, std::vector<size_t>> valid_pair{ };

void input_sequence(std::vector<size_t>& vec, std::chrono::microseconds time)
{
    sia::single_timer tm{ };
    for (size_t in{}; tm.get() <= time;)
    {
        if (r0.try_push_back(in))
        {   
            vec.emplace_back(in);
            ++in;
        }
        tm.now();
    }
}

void output_sequence(std::vector<size_t>& vec, std::chrono::microseconds time)
{
    sia::single_timer tm{ };
    for (size_t in{}; tm.get() <= time;)
    {
        if (r0.try_pop_front(in))
        {   
            vec.emplace_back(in);
        }
        tm.now();
    }
}

bool verify(std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>>& vv)
{
    for (auto& elem : vv)
    {
        valid_pair.first.insert(valid_pair.first.end(), elem.first.begin(), elem.first.end());
        valid_pair.second.insert(valid_pair.second.end(), elem.second.begin(), elem.second.end());
    }
    std::sort(valid_pair.first.begin(), valid_pair.first.end());
    std::sort(valid_pair.second.begin(), valid_pair.second.end());
    auto iiter{valid_pair.first.begin()}, oiter{valid_pair.second.begin()};
    while (iiter != valid_pair.first.end())
    {
        if (*iiter != *oiter)
        {
            std::print("hit\n");
            return false;
        }
        ++iiter;
        ++oiter;
    }
    return true;
}

int main()
{
    std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> vv{thread_num};
    for (auto& elem : vv)
    {
        std::thread input {input_sequence, std::ref(elem.first), input_time};
        std::thread output {output_sequence, std::ref(elem.second), output_time};
        input.detach();
        output.detach();
    }
    std::this_thread::sleep_for(output_time);
    bool result = verify(vv);
    return result;
}
