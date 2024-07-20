#pragma once

#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/align_wrapper.hpp"

// SPSC deque
namespace sia
{    
    namespace concurrency
    {
        namespace spsc
        {
            namespace deque_detail
            {                
                struct point
                {
                    false_share<std::atomic<size_t>> back;
                    false_share<std::atomic<size_t>> front;
                    false_share<size_t> shadow_back;
                    false_share<size_t> shadow_front;
                };
            } // namespace deque_detail

            template <typename T>
            struct deque
            {
            private:
                using point_t = deque_detail::point;

                point_t m_point;
                ring<T> m_ring;

                constexpr size_t size(size_t back, size_t front) noexcept { return back - front; }
                constexpr size_t full(size_t back, size_t front) noexcept { return size(back, front) == m_ring.capacity(); }
                constexpr size_t empty(size_t back, size_t front) noexcept { return size(back, front) == 0; }

                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) noexcept { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }

                template <typename C>
                constexpr void move_assign(C&& target, size_t pos) noexcept
                {
                    std::forward<C>(target) = std::move(m_ring[pos]);
                    m_ring[pos].~T();
                }

            public:
                constexpr deque(size_t size) noexcept : m_point(), m_ring(size) { }
                ~deque() noexcept
                {
                    for(auto& elem : m_ring)
                    {
                        elem.~T();
                    }
                }

                constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_relaxed) - m_point.front->load(std::memory_order_relaxed); }
                constexpr size_t full() noexcept { return size() == m_ring.capacity(); }
                constexpr size_t empty() noexcept { return size() == 0; }

                template <typename C>
                constexpr bool push_back(C&& arg) noexcept { return emplace_back(std::forward<C>(arg)); }

                template <typename... Cs>
                constexpr bool emplace_back(Cs&&... args) noexcept
                {
                    size_t real_pos = m_point.back->load(std::memory_order_relaxed);
                    if (full(real_pos, m_point.shadow_front.self()))
                    {
                        m_point.shadow_front.self() = m_point.front->load(std::memory_order_acquire);
                        if (full(real_pos, m_point.shadow_front.self()))
                        {
                            return false;
                        }
                    }
                    emplace(real_pos, std::forward<Cs>(args)...);
                    m_point.back->fetch_add(1, std::memory_order_relaxed);
                    return true;
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                    size_t real_pos = m_point.front->load(std::memory_order_relaxed);
                    if (empty(m_point.shadow_back.self(), real_pos))
                    {
                        m_point.shadow_back.self() = m_point.back->load(std::memory_order_acquire);
                        if (empty(m_point.shadow_back.self(), real_pos))
                        {
                            return false;
                        }
                    }
                    move_assign(std::forward<C>(arg), real_pos);
                    m_point.front->fetch_add(1, std::memory_order_relaxed);
                    return true;
                }
            };
            
        } // namespace spsc
    } // namespace concurrency
} // namespace sia



// MPMC deque
namespace sia
{
    namespace concurrency
    {
        namespace mpmc
        {
            namespace deque_detail
            {
                struct point
                {
                    false_share<std::atomic<size_t>> back;
                    false_share<std::atomic<size_t>> front;
                    false_share<std::atomic<size_t>> shadow_back;
                    false_share<std::atomic<size_t>> shadow_front;
                };

                struct flag_composit
                {
                    false_share<std::atomic_flag> m_own_back_flag;
                    false_share<std::atomic_flag> m_init_flag;
                    false_share<std::atomic_flag> m_own_front_flag;
                    false_share<std::atomic_flag> m_void_flag;
                };
            } // namespace deque_detail
            
            template <typename T>
            struct deque
            {
            private:
                using flag_t = deque_detail::flag_composit;
                using point_t = deque_detail::point;

                point_t m_point;
                ring<flag_t> m_flag;
                ring<T> m_ring;

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
                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) noexcept { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }
                template <typename C>
                constexpr void move_assign(size_t pos, C&& target) noexcept
                {
                    std::forward<C>(target) = std::move(m_ring[pos]);
                    m_ring[pos].~T();
                }

            public:
                constexpr deque(size_t size) noexcept : m_point{ }, m_flag{size}, m_ring{size}
                {
                    m_point.back->store(0);
                    m_point.front->store(0);
                    m_point.shadow_back->store(0);
                    m_point.shadow_front->store(0);
                    for (auto& elem : m_flag)
                    {
                        elem.m_own_back_flag->clear(std::memory_order_relaxed);
                        elem.m_init_flag->clear(std::memory_order_relaxed);
                        elem.m_own_front_flag->test_and_set(std::memory_order_relaxed);
                        elem.m_void_flag->test_and_set(std::memory_order_relaxed);
                    }
                }

                ~deque() noexcept
                {
                    for (auto& elem : m_ring)
                    {
                        elem.~T();
                    }
                }

                template <typename C>
                constexpr bool push_back(C&& arg) noexcept { return emplace_back(std::forward<C>(arg)); }

                template <typename... Cs>
                constexpr bool emplace_back(Cs&&... args) noexcept
                {
                    constexpr auto acq = std::memory_order_acquire;
                    constexpr auto rlx = std::memory_order_relaxed;
                    size_t shadow_pos = m_point.shadow_back->fetch_add(1, acq);
                    size_t temp_front = m_point.front->load(acq);
                    if (pushable(temp_front, shadow_pos))
                    {
                        // bullet
                        bool shadow_own = !m_flag[shadow_pos].m_own_back_flag->test_and_set(acq);
                        if (shadow_own)
                        {
                            size_t real_pos = m_point.back->fetch_add(1, acq);
                            // bool real_own = !m_flag[real_pos].m_own_back_flag->test_and_set(acq);
                            // if (real_own) { m_flag[shadow_pos].m_own_back_flag->clear(acq); }
                            // while (m_flag[real_pos].m_init_flag->test(acq)) { /*maybe*/ }
                            // while(!m_flag[real_pos].m_own_back_flag->test(acq)) { }
                            // while(!m_flag[real_pos].m_own_front_flag->test(acq)) { }
                            emplace(real_pos, std::forward<Cs>(args)...);
                            m_flag[real_pos].m_init_flag->test_and_set(acq);
                            // fire
                            m_flag[real_pos].m_own_front_flag->clear(acq);
                            return true;
                        }
                    }
                    m_point.shadow_back->fetch_sub(1, acq);
                    return false;
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                    constexpr auto acq = std::memory_order_acquire;
                    constexpr auto rlx = std::memory_order_relaxed;
                    size_t shadow_pos = m_point.shadow_front->fetch_add(1, acq);
                    size_t temp_back = m_point.back->load(acq);
                    if (popable(shadow_pos, temp_back))
                    {
                        // bullet
                        bool shadow_own = !m_flag[shadow_pos].m_own_front_flag->test_and_set(acq);
                        if (shadow_own)
                        {
                            size_t real_pos = m_point.front->fetch_add(1, acq);
                            // bool real_own = !m_flag[real_pos].m_own_front_flag->test_and_set(acq);
                            // if (real_own) { m_flag[shadow_pos].m_own_front_flag->clear(acq); }
                            // while (!m_flag[real_pos].m_init_flag->test(acq)) { /*maybe*/ }
                            // while(!m_flag[real_pos].m_own_front_flag->test(acq)) { }
                            // while(!m_flag[real_pos].m_own_back_flag->test(acq)) { }
                            move_assign(real_pos, std::forward<C>(arg));
                            m_flag[real_pos].m_init_flag->clear(acq);
                            // fire
                            m_flag[real_pos].m_own_back_flag->clear(acq);
                            return true;
                        }
                    }
                    m_point.shadow_front->fetch_sub(1, rlx);
                    return false;
                }
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia  
