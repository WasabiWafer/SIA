#pragma once

#include <memory>

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
                        false_share<std::atomic<size_t>> begin;
                        false_share<std::atomic<size_t>> end;
                        size_t shadow_begin;
                        size_t shadow_end;
                    }; // struct point
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
                        constexpr auto rlx = std::memory_order_relaxed;
                        size_t end = m_point.end->load(rlx);
                        if (full(m_point.shadow_begin, end))
                        {
                            m_point.shadow_begin = m_point.begin->load(rlx);
                            if (full(m_point.shadow_begin, end))
                            {
                                return false;
                            }
                        }
                        m_ring.emplace(end, std::forward<Cs>(args)...);
                        m_point.end->fetch_add(1, rlx);
                        return true;
                    }

                    template <typename C>
                    constexpr bool try_pop_front(C&& arg) noexcept {
                        constexpr auto rlx = std::memory_order_relaxed;
                        size_t begin = m_point.begin->load(rlx);
                        if (empty(begin, m_point.shadow_end))
                        {
                            m_point.shadow_end = m_point.end->load(rlx);
                            if (empty(begin, m_point.shadow_end))
                            {
                                return false;
                            }
                        }
                        arg = std::move(m_ring[begin]);
                        m_point.begin->fetch_add(1, rlx);
                        return true;
                    }
                }; // struct ring
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia


// lock free mpmc ring
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
                        false_share<std::atomic<size_t>> begin;
                        false_share<std::atomic<size_t>> end;
                    };
                } // namespace ring_detail
                
                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring 
                {
                private:
                    using point_t = ring_detail::point;
                    using ptr_alloc = std::allocator_traits<Allocator>::template rebind_alloc<false_share<std::atomic<T*>>>;

                    point_t m_point;
                    ::sia::ring<false_share<std::atomic<T*>>, Size, ptr_alloc> m_ptr;
                    ::sia::ring<T, Size, Allocator> m_ring;

                    constexpr size_t size(size_t begin_pos, size_t end_pos) noexcept { return end_pos - begin_pos; }
                    constexpr bool full(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == m_ring.capacity(); }
                    constexpr bool empty(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == 0; }
                public:
                    constexpr ring(const ring&) noexcept = delete;
                    constexpr ring& operator=(const ring&) noexcept = delete;
                    constexpr ring(const Allocator& alloc = Allocator()) noexcept
                        : m_point(), m_ptr(alloc), m_ring(alloc)
                    { }

                    template <typename C>
                    constexpr bool try_push_back(C&& arg) noexcept { return try_emplace_back(std::forward<C>(arg)); }
                    template <typename... Cs>
                    constexpr bool try_emplace_back(Cs&&... args) noexcept
                    {
                        constexpr auto rlx = std::memory_order_relaxed;
                        while (true)
                        {
                            size_t end = m_point.end->load(rlx);
                            if (!full(m_point.begin->load(rlx), end))
                            {
                                if (m_ptr[end]->load(rlx) == nullptr)
                                {
                                    if (m_point.end->compare_exchange_weak(end, end + 1, rlx, rlx))
                                    {
                                        m_ring.emplace(end, std::forward<Cs>(args)...);
                                        m_ptr[end]->store(m_ring.address(end), rlx);
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
                        constexpr auto rlx = std::memory_order_relaxed;
                        while (true)
                        {
                            size_t begin = m_point.begin->load(rlx);
                            if (!empty(begin, m_point.end->load(rlx)))
                            {
                                if (m_ptr[begin]->load(rlx) != nullptr)
                                {
                                    if (m_point.begin->compare_exchange_weak(begin, begin + 1, rlx, rlx))
                                    {
                                        arg = std::move(*(m_ptr[begin]->load(rlx)));
                                        m_ptr[begin]->store(nullptr, rlx);
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
                }; // struct ring
            } // namespace mpmc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia