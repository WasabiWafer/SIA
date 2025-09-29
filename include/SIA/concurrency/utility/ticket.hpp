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
        private:
            using value_type = T;
            using atomic_type = std::atomic<value_type>;
            true_share<atomic_type> m_ontic;
            true_share<atomic_type> m_offtic;
            static value_type max() noexcept { return std::numeric_limits<value_type>::max(); }
        public:
            
    };
} // namespace sia
