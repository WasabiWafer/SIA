#pragma once

#include <memory>
#include <mutex>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/align_wrapper.hpp"

// lock_free spsc ring
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
                    struct point
                    {
                        false_share<std::atomic<size_t>> front;
                        false_share<std::atomic<size_t>> back;
                        size_t shadow_front;
                        size_t shadow_back;
                    };
                } // namespace ring_detail

                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring {
                private:
                    using point_t = ring_detail::point;
                    point_t m_point;
                    ::sia::ring<T, Size, Allocator> m_ring;

                    constexpr size_t size(size_t begin_pos, size_t end_pos) noexcept { return end_pos - begin_pos; }
                    constexpr bool full(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == m_ring.capacity(); }
                    constexpr bool empty(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == 0; }
                public:
                    constexpr ring(const ring&) noexcept = delete;
                    constexpr ring& operator=(const ring&) noexcept = delete;
                    constexpr ring(const Allocator& alloc = Allocator()) noexcept
                        : m_point(), m_ring(alloc)
                    { }

                    template <typename C>
                    constexpr bool try_push_back(C&& arg) noexcept {
                        return try_emplace_back(std::forward<C>(arg));
                    }

                    template <typename... Cs>
                    constexpr bool try_emplace_back(Cs&&... args) noexcept {
                        constexpr auto acq = std::memory_order_acquire;
                        constexpr auto rlx = std::memory_order_relaxed;
                        size_t back = m_point.back->load(rlx);
                        if (full(m_point.shadow_front, back))
                        {
                            m_point.shadow_front = m_point.front->load(acq);
                            if (full(m_point.shadow_front, back))
                            {
                                return false;
                            }
                        }
                        m_ring.emplace(back, std::forward<Cs>(args)...);
                        m_point.back->fetch_add(1, rlx);
                        return true;
                    }

                    template <typename C>
                    constexpr bool try_pop_front(C&& arg) noexcept {
                        constexpr auto acq = std::memory_order_acquire;
                        constexpr auto rlx = std::memory_order_relaxed;
                        size_t front = m_point.front->load(rlx);
                        if (empty(front, m_point.shadow_back))
                        {
                            m_point.shadow_back = m_point.back->load(acq);
                            if (empty(front, m_point.shadow_back))
                            {
                                return false;
                            }
                        }
                        arg = std::move(m_ring[front]);
                        m_point.front->fetch_add(1, rlx);
                        return true;
                    }
                };
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia


// lock free mpmc ring
// currently mpmc ring is not safe lock free
namespace sia
{
    namespace concurrency
    {
        namespace lock_free
        {
            namespace mpmc
            {
                namespace ring_detail
                {
                    struct point
                    {
                        false_share<std::atomic<size_t>> front;
                        false_share<std::atomic<size_t>> back;
                    };
                } // namespace ring_detail
                
                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring 
                {
                private:
                    using point_t = ring_detail::point;
                    using flag_t = false_share<std::atomic<size_t>>;
                    using flag_alloc = std::allocator_traits<Allocator>::template rebind_alloc<flag_t>;
                    point_t m_point;
                    ::sia::ring<flag_t, Size, flag_alloc> m_flag;
                    ::sia::ring<T, Size, Allocator> m_ring;

                    constexpr size_t size(size_t begin_pos, size_t end_pos) noexcept { return end_pos - begin_pos; }
                    constexpr bool full(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == m_ring.capacity(); }
                    constexpr bool empty(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == 0; }
                public:
                    constexpr ring(const Allocator& alloc = Allocator()) noexcept
                        : m_point(), m_flag(alloc), m_ring(alloc)
                    { }

                    template <typename C>
                    constexpr bool try_push_back(C&& arg) noexcept { return try_emplace_back(std::forward<C>(arg)); }
                    template <typename... Cs>
                    constexpr bool try_emplace_back(Cs&&... args) noexcept
                    {
                        constexpr auto acq = std::memory_order_acquire;
                        constexpr auto rle = std::memory_order_release;
                        constexpr auto rlx = std::memory_order_relaxed;
                        while(true)
                        {
                            size_t back_pos = m_point.back->load(rlx);
                            if (!full(m_point.front->load(rlx), back_pos))
                            {
                                if (m_flag[back_pos]->load(rlx) == 0)
                                {
                                    if (m_point.back->compare_exchange_weak(back_pos, back_pos + 1, rlx, rlx))
                                    {
                                        m_ring.emplace(back_pos, std::forward<Cs>(args)...);
                                        m_flag[back_pos]->store(1, rlx);
                                        return true;
                                    }
                                }
                                else
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }

                    template <typename C>
                    constexpr bool try_pop_front(C&& arg) noexcept
                    {
                        constexpr auto acq = std::memory_order_acquire;
                        constexpr auto rle = std::memory_order_release;
                        constexpr auto rlx = std::memory_order_relaxed;
                        while(true)
                        {
                            size_t front_pos = m_point.front->load(rlx);
                            if (!empty(front_pos, m_point.back->load(rlx)))
                            {
                                if (m_flag[front_pos]->load(rlx) == 1)
                                {
                                    if (m_point.front->compare_exchange_weak(front_pos, front_pos + 1, rlx, rlx))
                                    {
                                        arg = std::move(m_ring[front_pos]);
                                        m_flag[front_pos]->store(0, rlx);
                                        return true;
                                    }
                                }
                                else
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                };
            } // namespace mpmc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia

