#pragma once

#include <thread>

#include "SIA/utility/align_wrapper.hpp"

namespace sia
{
    static_assert(std::is_trivially_copyable_v<std::thread::id> && std::is_standard_layout_v<std::thread::id>);
    using thread_id_t = unsigned_integer_t<sizeof(std::thread::id)>;

    template <typename T>
    using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename T>
    using true_share  = align_wrapper<T, std::hardware_constructive_interference_size>;
} // namespace sia
