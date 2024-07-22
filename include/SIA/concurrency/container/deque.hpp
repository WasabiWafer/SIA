#pragma once

#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/align_wrapper.hpp"
#include "SIA/concurrency/mutex.hpp"

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
                        false_share<std::atomic<size_t>> back;
                        false_share<std::atomic<size_t>> front;
                        size_t shadow_back;
                        size_t shadow_front;
                    };
                } // namespace deque_detail

                template <typename T>
                struct deque
                {
                private:
                    using point_t = deque_detail::point;
                    point_t m_point;
                    ring<T> m_ring;
                    template <typename... Cs>
                    constexpr void emplace(size_t pos, Cs&&... args) noexcept { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }
                    template <typename C>
                    constexpr void move_assign(size_t pos, C&& target) noexcept { target = std::move(m_ring[pos]); m_ring[pos].~T(); }
                    constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_acquire) - m_point.front->load(std::memory_order_acquire); }
                    constexpr bool empty() noexcept { return size() == 0; }
                    constexpr bool full() noexcept { return size() == m_ring.capacity(); }
                    constexpr bool pushable() noexcept { return !full(); }
                    constexpr bool popable() noexcept { return !empty(); }
                    constexpr size_t size(size_t front, size_t back) noexcept { return back - front; }
                    constexpr bool empty(size_t front, size_t back) noexcept { return size(front, back) == 0; }
                    constexpr bool full(size_t front, size_t back) noexcept { return size(front, back) >= m_ring.capacity(); }
                    constexpr bool pushable(size_t front, size_t back) noexcept { return verify_pos(front, back) ? !full(front, back) : false; }
                    constexpr bool popable(size_t front, size_t back) noexcept { return verify_pos(front, back) ? !empty(front, back) : false; }
                    constexpr bool verify_pos(size_t front, size_t back) noexcept
                    {
                        // 3 way checking
                        size_t arg0 = size(front, back);
                        size_t arg1 = size(0, back - front);
                        size_t arg2 = size(front - back, 0);
                        bool is_valid = ((arg0 == arg1) & (arg0 == arg2) & (arg1 == arg2));
                        bool is_able = ((arg0 <= m_ring.capacity()) | (arg1 <= m_ring.capacity()) | (arg2 <= m_ring.capacity()));
                        if (is_valid & is_able)
                        {
                            return true;
                        }
                        return false;
                    }

                public:
                    constexpr deque(size_t size) noexcept : m_point(), m_ring(size) { }
                    ~deque() noexcept { for(auto& elem : m_ring) { elem.~T(); } }

                    template <typename C>
                    constexpr bool push_back(C&& arg) noexcept { return emplace_back(std::forward<C>(arg)); }
                    template <typename... Cs>
                    constexpr bool emplace_back(Cs&&... args) noexcept
                    {
                        size_t back = m_point.back->load(std::memory_order_relaxed);
                        if (full(m_point.shadow_front, back))
                        {
                            m_point.shadow_front = m_point.front->load(std::memory_order_acquire);
                            if (full(m_point.shadow_front, back))
                            {
                                return false;
                            }
                        }
                        emplace(back, std::forward<Cs>(args)...);
                        m_point.back->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }

                    template <typename C>
                    constexpr bool pop_front(C&& arg) noexcept
                    {
                        size_t front = m_point.front->load(std::memory_order_relaxed);
                        if (empty(front, m_point.shadow_back))
                        {
                            m_point.shadow_back = m_point.back->load(std::memory_order_acquire);
                            if (empty(front, m_point.shadow_back))
                            {
                                return false;
                            }
                        }
                        move_assign(front, std::forward<C>(arg));
                        m_point.front->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                };
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia


namespace sia
{
    namespace concurrency
    {
        namespace mpmc
        {
            namespace deque_detail
            {
                struct mtx_composit
                {
                    mutex m_own_front;
                    mutex m_own_back;
                    std::atomic_flag m_init;
                };
            } // namespace deque_detail
            
            template <typename T>
            struct deque
            {
                
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia

