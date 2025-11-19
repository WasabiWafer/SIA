#pragma once

#include <atomic>

namespace sia
{
    namespace tags
    {
        enum class lever { off, on };
    } // namespace tags
    
    template <typename = void>
        requires (std::atomic<tags::lever>::is_always_lock_free)
    struct lever
    {
        private:
            using atomic_type = std::atomic<tags::lever>;
            atomic_type m_state;
        public:
            constexpr tags::lever status(std::memory_order mem_order = std::memory_order::seq_cst) noexcept { return m_state.load(mem_order); }
            constexpr tags::lever action(std::memory_order rmw_mem_order = std::memory_order::seq_cst, std::memory_order load_mem_order = std::memory_order::seq_cst) noexcept
            {
                tags::lever expt {status(load_mem_order)};
                tags::lever desr { };
                do
                {
                    if (expt == tags::lever::off)
                    { desr = tags::lever::on; }
                    else if (expt == tags::lever::on)
                    { desr = tags::lever::off; }
                }
                while (!m_state.compare_exchange_weak(expt, desr, rmw_mem_order, load_mem_order));
                return desr;
            }
    };
} // namespace sia
