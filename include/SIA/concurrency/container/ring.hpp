#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/align_wrapper.hpp"

// lock_free spsc deque
namespace sia
{    
    namespace concurrency
    {
        namespace lock_free
        {
            namespace spsc
            {
                namespace deque_detail
                {
                    struct point
                    {
                        false_share<std::atomic<size_t>> front;
                        false_share<std::atomic<size_t>> back;
                        size_t shadow_front;
                        size_t shadow_back;
                    };
                } // namespace deque_detail

                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring {
                private:
                    using point_t = deque_detail::point;
                    point_t m_point;
                    ::sia::ring<T, Size, Allocator> m_ring;

                    constexpr size_t size(size_t begin_pos, size_t end_pos) noexcept { return end_pos - begin_pos; }
                    constexpr bool full(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == m_ring.capacity(); }
                    constexpr bool empty(size_t begin_pos, size_t end_pos) noexcept { return size(begin_pos, end_pos) == 0; }
                public:
                    constexpr ring(const ring&) noexcept = delete;
                    constexpr ring& operator=(const ring&) noexcept = delete;
                    constexpr ring() noexcept
                        : m_point(), m_ring()
                    { }
                    constexpr ring(const Allocator& alloc) noexcept
                        : m_point(), m_ring(compressed_pair_tag::one, alloc)
                    { }

                    template <typename C>
                    constexpr bool try_push_back(C&& arg) noexcept {
                        return try_emplace_back(std::forward<C>(arg));
                    }

                    template <typename... Cs>
                    constexpr bool try_emplace_back(Cs&&... args) noexcept {
                        constexpr auto acq = std::memory_order_acquire;
                        constexpr auto rlx = std::memory_order_relaxed;
                        constexpr auto rle = std::memory_order_release;
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
                        constexpr auto rle = std::memory_order_release;
                        size_t front = m_point.front->load(rlx);
                        if (empty(front, m_point.shadow_back))
                        {
                            m_point.shadow_back = m_point.back->load(acq);
                            if (empty(front, m_point.shadow_back))
                            {
                                return false;
                            }
                        }
                        std::forward<C>(arg) = std::move(m_ring[front]);
                        m_ring.destroy(front);
                        return true;
                    }
                };
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
