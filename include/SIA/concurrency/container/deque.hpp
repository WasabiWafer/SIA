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
                    false_share<std::atomic_flag> m_back_own_flag;
                    false_share<std::atomic_flag> m_front_own_flag;
                    false_share<std::atomic_flag> m_own_flag;
                    false_share<std::atomic_flag> m_init_flag;
                    false_share<std::atomic<size_t>> m_flag;
                };

                enum : size_t
                {
                    
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

                constexpr size_t size(size_t front, size_t back) noexcept { return (back - front); }
                constexpr bool full(size_t front, size_t back) noexcept { return size(front, back) == m_ring.capacity(); }
                constexpr bool empty(size_t front, size_t back) noexcept { return size(front, back) == 0; }
                constexpr bool verify_pos(size_t front, size_t back) noexcept { return size(front, back) <= m_ring.capacity() ? true :  false; }
                constexpr bool pushable(size_t front, size_t back) noexcept { return verify_pos(front, back) ? full(front, back) : false; }
                constexpr bool popable(size_t front, size_t back) noexcept { return verify_pos(front, back) ? empty(front, back) : false; }
                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) noexcept { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }
                template <typename C>
                constexpr void move_assign(size_t pos, C&& target) noexcept
                {
                    std::forward<C>(target) = std::move(m_ring[pos]);
                    m_ring[pos].~T();
                }

                template <typename... Cs>
                constexpr void input(Cs&&... args)
                {
                    size_t pos = m_point.back->fetch_add(1, std::memory_order_relaxed);
                    emplace(pos, std::forward<Cs>(args));
                }

                template <typename C>
                constexpr void output(C&& arg)
                {
                    size_t pos = m_point.front->fetch_add(1, std::memory_order_relaxed);
                    move_assign(pos, std::forward<C>(arg));
                }

                constexpr bool shadow_back_owning(size_t& arg) noexcept
                {
                    size_t shadow_pos { };
                    bool owned {false};
                    while (!owned)
                    {
                        shadow_pos = m_point.shadow_back->load(std::memory_order_relaxed);
                        if (pushable(m_point.front->load(std::memory_order_relaxed), shadow_pos))
                        {
                            owned = !m_flag[shadow_pos].m_back_own_flag->test_and_set(std::memory_order_relaxed);
                            if (owned)
                            {
                                arg = m_point.shadow_back->fetch_add(1, std::memory_order_relaxed);
                                return true;
                            }
                        }
                        else { return false; }
                    }
                    return false;
                }

                template <typename T>
                constexpr bool push_owning(size_t& ret, T&& atm) noexcept
                {
                    size_t pos{ };
                    bool owned {false};
                    while (!owned)
                    {
                        pos = atm.load(std::memory_order_relaxed);
                        if (pushable(m_point.front->load(std::memory_order_relaxed), pos))
                        {
                            owned = !m_flag[pos].m_back_own_flag->test_and_set(std::memory_order_relaxed);
                            if (owned)
                            {
                                ret = atm.fetch_add(1, std::memory_order_relaxed);
                                return true;
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                }

                template <typename T>
                constexpr bool pop_owning(size_t& ret, T&& atm) noexcept
                {
                    size_t pos{ };
                    bool owned {false};
                    while (!owned)
                    {
                        pos = atm.load(std::memory_order_relaxed);
                        if (popable(pos, m_point.back->load(std::memory_order_relaxed)))
                        {
                            owned = !m_flag[pos].m_front_own_flag->test_and_set(std::memory_order_relaxed);
                            if (owned)
                            {
                                ret = atm.fetch_add(1, std::memory_order_relaxed);
                                return true;
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                }

            public:
                constexpr deque(size_t size) noexcept : m_point(), m_flag(size), m_ring(size) { }

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
                    size_t shadow_pos { };
                    if (push_owning(shadow_pos, m_point.shadow_back.self()))
                    {
                        emplace(shadow_pos, std::forward<Cs>(args)...);
                        m_flag[shadow_pos].m_init_flag->test_and_set(std::memory_order_relaxed);
                        if (!m_flag[shadow_pos].m_front_own_flag->test_and_set(std::memory_order_relaxed))
                        { m_flag[shadow_pos].m_front_own_flag->clear(std::memory_order_relaxed); }
                        m_point.back->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                   size_t shadow_pos { };
                   if (pop_owning(shadow_pos, m_point.shadow_front.self()))
                   {
                        while (!m_flag[shadow_pos].m_init_flag->test()) { std::this_thread::yield() }
                        move_assign(shadow_pos, std::forward<C>(arg));
                        m_flag[shadow_pos].m_init_flag->clear(std::memory_order_relaxed);
                        if (!m_flag[shadow_pos].m_back_own_flag->test_and_set(std::memory_order_relaxed))
                        { m_flag[shadow_pos].m_back_own_flag->clear(std::memory_order_relaxed); }
                        m_point.front->fetch_add(1, std::memory_order_relaxed);
                        return true;
                   }
                   else
                   {
                        return false;
                   }
                }
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia  
