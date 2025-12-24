#pragma once

#include <limits>
#include <atomic>

#include "SIA/internals/types.hpp"

namespace sia
{
    template <typename T = largest_unsigned_integer_t, T Max = std::numeric_limits<T>::max()>
        requires (std::atomic<T>::is_always_lock_free)
    struct scalero
    {
        private:
            using value_type = T;
            using atomic_type = std::atomic<value_type>;
            atomic_type m_num;

        public:
            constexpr value_type status(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { return m_num.load(mem_order); }
            
            constexpr value_type action(std::memory_order rmw_order = std::memory_order::seq_cst, std::memory_order load_order = std::memory_order::seq_cst) noexcept
            {
                value_type tmp = m_num.load(load_order);
                while(!m_num.compare_exchange_weak(tmp, (tmp+1)%Max, rmw_order, load_order)) {}
                return tmp;
            }
    };
} // namespace sia