#pragma once

#include <thread>

namespace sia
{
    using thread_id_t = decltype(std::declval<std::thread::id>()._Get_underlying_id());
} // namespace sia
