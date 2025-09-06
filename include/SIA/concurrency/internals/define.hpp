#pragma once

#include <atomic>

#include "SIA/utility/tools.hpp"
#include "SIA/concurrency/internals/types.hpp"

namespace sia
{    
    namespace tags
    {
        enum class producer { single, multiple };
        enum class consumer { single, multiple };
    }
    
    namespace stamps
    {
        namespace memory_orders
        {
            constexpr const auto seq_cst_v = std::memory_order_seq_cst;
            constexpr const auto relaxed_v = std::memory_order_relaxed;
            constexpr const auto acquire_v = std::memory_order_acquire;
            constexpr const auto consume_v = std::memory_order_consume;
            constexpr const auto release_v = std::memory_order_release;
            constexpr const auto acq_rel_v = std::memory_order_acq_rel;
        } // namespace memory_order

        namespace this_thread
        {
            thread_local const thread_id_t id_v = type_cast<thread_id_t>(std::this_thread::get_id());
        } // namespace thread
    } // namespace tag
} // namespace sia
