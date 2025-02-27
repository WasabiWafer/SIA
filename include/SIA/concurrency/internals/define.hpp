#pragma once

#include <atomic>

namespace sia
{
    namespace stamps
    {
        namespace memory_orders
        {
            constexpr auto seq_cst_v = std::memory_order_seq_cst;
            constexpr auto relaxed_v = std::memory_order_relaxed;
            constexpr auto acquire_v = std::memory_order_acquire;
            constexpr auto consume_v = std::memory_order_consume;
            constexpr auto release_v = std::memory_order_release;
            constexpr auto acq_rel_v = std::memory_order_acq_rel;
        } // namespace memory_order
    } // namespace tag
} // namespace sia
