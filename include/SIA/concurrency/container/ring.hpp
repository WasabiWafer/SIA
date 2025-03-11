#pragma once

#include <memory>
#include <atomic>

#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/compressed_pair.hpp"
#include "SIA/concurrency/utility/mutex.hpp"
#include "SIA/concurrency/utility/semaphore.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace lock_free
        {
            namespace spsc
            {
                namespace ring_detail
                {
                    template <typename T>
                    struct ring_composition
                    {
                        semaphore<1> m_sem_back;
                        semaphore<1> m_sem_front;
                        false_share<std::atomic<T*>> m_data;
                        false_share<std::atomic<size_t>> m_begin;
                        false_share<std::atomic<size_t>> m_end;
                    };
                } // namespace ring_detail
                
                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring
                {
                private:
                    using allocator_t = Allocator;
                    using allocator_traits_t = std::allocator_traits<Allocator>;
                    using composition_t = ring_detail::ring_composition<T>;
                    compressed_pair<allocator_t, composition_t> m_compair;

                    constexpr composition_t& get_compair() noexcept { return this->m_compair.second(); }
                    constexpr const composition_t& get_compair() const noexcept { return this->m_compair.second(); }
                    constexpr allocator_t&  get_alloc() noexcept { return this->m_compair.first(); }
                    constexpr const allocator_t&  get_alloc() const noexcept { return this->m_compair.first(); }
                public:
                    constexpr ring(const allocator_t& alloc) noexcept(noexcept(allocator_traits_t::allocate(this->get_alloc(), Size)))
                        : m_compair(splits::one_v, alloc)
                    { this->get_compair().m_data = allocator_traits_t::allocate(this->get_alloc(), Size); }
                };
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
