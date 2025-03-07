#pragma once

#include <thread>

#include "SIA/utility/align_wrapper.hpp"

namespace sia
{
    using thread_id_t = decltype(std::declval<std::thread::id>()._Get_underlying_id());

    template <typename T>
    using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename T>
    using true_share  = align_wrapper<T, std::hardware_constructive_interference_size>;
} // namespace sia
