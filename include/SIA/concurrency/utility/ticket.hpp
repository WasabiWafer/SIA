#pragma once

#include <atomic>
#include <limits>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/internals/types.hpp"

namespace sia
{
    template <typename T = largest_unsigned_integer_t>
        requires (std::atomic<T>::is_always_lock_free)
    struct ticket
    {
        public:
            using value_type = T;
        private:
            using atomic_type = std::atomic<value_type>;
            true_share<atomic_type> m_ticket;
            true_share<atomic_type> m_check;
            static value_type max() noexcept { return std::numeric_limits<value_type>::max(); }
            static value_type step() noexcept { return value_type{1}; }
        public:
            constexpr value_type check_in(std::memory_order mem_order = std::memory_order::seq_cst) noexcept { return m_ticket->fetch_add(step(), mem_order); }
            constexpr bool check(value_type num, std::memory_order mem_order = std::memory_order::seq_cst) noexcept { return m_check->load(mem_order) == num; }
            constexpr void check_out(std::memory_order mem_order = std::memory_order::seq_cst) noexcept { m_check->fetch_add(step(), mem_order); }
    };
} // namespace sia
