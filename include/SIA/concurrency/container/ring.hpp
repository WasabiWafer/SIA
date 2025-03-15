#pragma once
#include <print>
#include <memory>
#include <atomic>
#include <optional>

#include "SIA/utility/align_wrapper.hpp"
#include "SIA/utility/compressed_pair.hpp"
#include "SIA/concurrency/utility/tools.hpp"
#include "SIA/concurrency/utility/mutex.hpp"
#include "SIA/concurrency/utility/quota.hpp"

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
                    false_share<std::optional<T>*> m_data;
                    false_share<mutex> m_mu_pro;
                    false_share<mutex> m_mu_con;
                    false_share<std::atomic<size_t>> m_begin;
                    false_share<std::atomic<size_t>> m_end;
                };
            } // namespace ring_detail
            
            template <typename T, size_t Size, typename Allocator = std::allocator<T>>
            struct ring
            {
            private:
                using value_t = std::optional<T>;
                using allocator_t = std::allocator_traits<Allocator>::template rebind_alloc<value_t>;
                using allocator_traits_t = std::allocator_traits<allocator_t>;
                using composition_t = ring_detail::ring_composition<T>;
            public:
                compressed_pair<allocator_t, composition_t> m_compair;

                constexpr composition_t& get_comp() noexcept { return this->m_compair.second(); }
                constexpr allocator_t& get_alloc() noexcept { return this->m_compair.first(); }
                constexpr value_t* get_data() noexcept { return this->m_compair.second().m_data.ref(); }
                constexpr value_t* raw_address(size_t pos) noexcept { return this->get_data() + (pos % this->capacity());}
                
            public:
                constexpr ring(const allocator_t& alloc = allocator_t()) noexcept(noexcept(allocator_t(alloc)) && noexcept(allocator_traits_t::allocate(this->get_alloc(), Size)))
                    : m_compair(splits::one_v, alloc)
                {
                    constexpr auto mem_order = stamps::memory_orders::acq_rel_v;
                    composition_t& comp = this->get_comp();
                    allocator_t& allocator = this->get_alloc();
                    comp.m_data.ref() = allocator_traits_t::allocate(allocator, Size);
                    for (size_t n { }; n < Size; ++n)
                    { allocator_traits_t::construct(allocator, comp.m_data.ref() + n); }
                }

                ~ring() noexcept(noexcept(allocator_traits_t::deallocate(this->get_alloc(), this->get_data(), this->capacity())))
                { allocator_traits_t::deallocate(this->get_alloc(), this->get_data(), this->capacity()); }

                constexpr size_t capacity(this auto&& self) noexcept { return Size; }
                constexpr size_t size(this auto&& self) noexcept
                {
                    constexpr auto mem_order = stamps::memory_orders::acquire_v;
                    composition_t& comp = self.get_comp();
                    return comp.m_end->load(mem_order) - comp.m_begin->load(mem_order);
                }
                constexpr bool is_empty(this auto&& self) noexcept
                { return self.size() == 0; }
                constexpr bool is_full(this auto&& self) noexcept
                { return self.capacity() == self.size(); }

                constexpr bool try_pop_back(T& out) noexcept(noexcept(out = T()) && noexcept(value_t().~value_t()))
                {
                    constexpr auto acq = stamps::memory_orders::acquire_v;
                    constexpr auto rle = stamps::memory_orders::release_v;
                    composition_t& comp = this->get_comp();
                    quota q {comp.m_mu_con.ref(), tags::quota::try_take};
                    if (q.is_own())
                    {
                        if (!this->is_empty())
                        {
                            value_t* target_ptr = this->raw_address(comp.m_end->load(acq) - 1);
                            out = std::move(target_ptr->value());
                            target_ptr->reset();
                            comp.m_end->fetch_sub(1, rle);
                            return true;
                        }
                    }
                    return false;
                }

                template <typename... Tys>
                constexpr bool try_emplace_back(Tys&&... args) noexcept(noexcept(T(std::forward<Tys>(args)...)))
                {
                    constexpr auto acq = stamps::memory_orders::acquire_v;
                    constexpr auto rle = stamps::memory_orders::release_v;
                    composition_t& comp = this->get_comp();
                    quota q {comp.m_mu_pro.ref(), tags::quota::try_take};
                    if (q.is_own())
                    {
                        if (!this->is_full())
                        {
                            value_t* target_ptr = this->raw_address(comp.m_end->load(acq));
                            target_ptr->emplace(std::forward<Tys>(args)...);
                            comp.m_end->fetch_add(1, rle);
                            return true;
                        }
                    }
                    return false;
                }

                constexpr bool try_push_back(const T& arg) noexcept(noexcept(this->try_emplace_back(arg)))
                { return this->try_emplace_back(arg); }
                constexpr bool try_push_back(T&& arg) noexcept(noexcept(this->try_emplace_back(std::move(arg))))
                { return this->try_emplace_back(std::move(arg)); }

                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void pop_back(T& out) noexcept(noexcept(this->try_pop_back(out)))
                { loop<true, LoopTag>(&ring::try_pop_back, this, out); }

                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void push_back(const T& arg) noexcept(noexcept(this->try_push_back(arg)))
                { loop<true, LoopTag>(static_cast<bool(ring::*)(const T&)>(&ring::try_push_back), this, arg); }
                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void push_back(T&& arg) noexcept(noexcept(this->try_push_back(std::move(arg))))
                { loop<true, LoopTag>(static_cast<bool(ring::*)(T&&)>(&ring::try_push_back), this, std::move(arg)); }

                template <tags::wait LoopTag = tags::wait::busy, typename... Tys>
                constexpr void emplace_back(Tys&&... args) noexcept(noexcept(this->try_emplace_back(std::forward<Tys>(args)...)))
                { loop<true, LoopTag>(&ring::try_emplace_back<Tys...>, this, std::forward<Tys>(args)...); }
                
                constexpr bool try_pop_front(T& out) noexcept(noexcept(out = T()) && noexcept(value_t().~value_t()))
                {
                    constexpr auto acq = stamps::memory_orders::acquire_v;
                    constexpr auto rle = stamps::memory_orders::release_v;
                    composition_t& comp = this->get_comp();
                    quota q {comp.m_mu_con.ref(), tags::quota::try_take};
                    if (q.is_own())
                    {
                        if (!this->is_empty())
                        {
                            value_t* target_ptr = this->raw_address(comp.m_begin->load(acq));
                            out = target_ptr->value();
                            target_ptr->reset();
                            comp.m_begin->fetch_add(1, rle);
                            return true;
                        }
                    }
                    return false;
                }

                template <typename... Tys>
                constexpr bool try_emplace_front(Tys&&... args) noexcept(noexcept(T(std::forward<Tys>(args)...)))
                {
                    constexpr auto acq = stamps::memory_orders::acquire_v;
                    constexpr auto rle = stamps::memory_orders::release_v;
                    composition_t& comp = this->get_comp();
                    quota q {comp.m_mu_pro.ref(), tags::quota::try_take};
                    if (q.is_own())
                    {
                        if (!this->is_full())
                        {
                            value_t* target_ptr = this->raw_address(comp.m_begin->load(acq) - 1);
                            target_ptr->emplace(std::forward<Tys>(args)...);
                            comp.m_begin->fetch_sub(1, rle);
                            return true;
                        }
                    }
                    return false;
                }

                constexpr bool try_push_front(const T& arg) noexcept(noexcept(this->try_emplace_front(arg)))
                { return this->try_emplace_front(arg); }
                constexpr bool try_push_front(T&& arg) noexcept(noexcept(this->try_emplace_front(std::move(arg))))
                { return this->try_emplace_front(std::move(arg)); }
                
                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void pop_front(T& out) noexcept(noexcept(this->try_pop_front(out)))
                { loop<true, LoopTag>(&ring::try_pop_front, this, out); }

                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void push_front(const T& arg) noexcept(noexcept(this->try_push_front(arg)))
                { loop<true, LoopTag>(static_cast<bool(ring::*)(const T&)>(&ring::try_push_front), this, arg); }
                template <tags::wait LoopTag = tags::wait::busy>
                constexpr void push_front(T&& arg) noexcept(noexcept(this->try_push_front(std::move(arg))))
                { loop<true, LoopTag>(static_cast<bool(ring::*)(T&&)>(&ring::try_push_front), this, std::move(arg)); }

                template <tags::wait LoopTag = tags::wait::busy, typename... Tys>
                constexpr void emplace_front(Tys&&... args) noexcept(noexcept(this->try_emplace_front(std::forward<Tys>(args)...)))
                { loop<true, LoopTag>(&ring::try_emplace_front<Tys...>, this, std::forward<Tys>(args)...); }
            };
        } // namespace spsc

        namespace mpmc
        {
            
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia
