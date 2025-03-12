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
        namespace spsc
        {
            namespace ring_detail
            {
                template <typename T>
                struct ring_composition
                {
                    false_share<std::atomic<T*>> m_data;
                    false_share<semaphore<1>> m_sep_begin;
                    false_share<std::atomic<size_t>> m_begin;
                    false_share<semaphore<1>> m_sep_end;
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

                constexpr composition_t& get_comp() noexcept { return this->m_compair.second(); }
                constexpr allocator_t&  get_alloc() noexcept { return this->m_compair.first(); }
                
            public:
                constexpr ring(const allocator_t& alloc) noexcept(noexcept(allocator_traits_t::allocate(this->get_alloc(), Size)))
                    : m_compair(splits::one_v, alloc)
                { this->get_comp().m_data = allocator_traits_t::allocate(this->get_alloc(), Size); }

                ~ring() noexcept(noexcept(allocator_t::deallocate(this->get_alloc(), this->get_comp().m_data, this->capacity())))
                { allocator_t::deallocate(this->get_alloc(), this->get_comp().m_data, this->capacity()); }

                constexpr size_t capacity(this auto&& self) noexcept { return Size; }
                constexpr size_t size(this auto&& self) noexcept
                {
                    constexpr auto mem_order = stamps::memory_orders::relaxed_v;
                    composition_t& comp = self.get_comp();
                    return comp.m_end->load(mem_order) - comp.m_begin->load(mem_order);
                }

                template <typename... Tys>
                constexpr bool try_emplace_back(Tys&&... args) noexcept
                {
                    composition_t& comp = self.get_comp();
                    bool cond {comp.m_sep_begin->try_acquire()}; // need lock/acq
                    return cond;
                }
            };
        } // namespace spsc
    } // namespace concurrency
} // namespace sia
