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
                    false_share<size_t> sav_back;
                    false_share<size_t> sav_front;
                };
            } // namespace deque_detail

            template <typename T>
            struct deque
            {
            private:
                using point_t = deque_detail::point;

                point_t m_point;
                ring<T> m_ring;

                deque(const deque&) = delete;
                deque& operator=(const deque&) = delete;

                constexpr bool full(size_t back_pos, size_t front_pos) noexcept { return back_pos >= (front_pos + m_ring.capacity()); }
                constexpr bool empty(size_t back_pos, size_t front_pos) noexcept { return back_pos <= front_pos; }
                constexpr bool full() noexcept { return m_point.back->load(std::memory_order_acquire) >= (m_point.front->load(std::memory_order_acquire) + m_ring.capacity()); }
                constexpr bool empty() noexcept { return m_point.back->load(std::memory_order_acquire) <= m_point.front->load(std::memory_order_acquire); }
                constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_relaxed) - m_point.front->load(std::memory_order_relaxed); }

                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }

                constexpr std::tuple<bool, size_t> get_back_pos() noexcept
                {
                    size_t back_pos = m_point.back->load(std::memory_order_relaxed);
                    if (full(back_pos, m_point.sav_front.self()))
                    {
                        m_point.sav_front.self() = m_point.front->load(std::memory_order_acquire);
                        if (full(back_pos, m_point.sav_front.self()))
                        {
                            return {false, 0};
                        }
                    }
                    return {true, back_pos};
                }

                constexpr std::tuple<bool, size_t> get_front_pos() noexcept
                {
                    size_t front_pos = m_point.front->load(std::memory_order_relaxed);
                    if (empty(m_point.sav_back.self(), front_pos))
                    {
                        m_point.sav_back.self() = m_point.back->load(std::memory_order_acquire);
                        if (empty(m_point.sav_back.self(), front_pos))
                        {
                            return {false, 0};
                        }
                    }
                    return {true, front_pos};
                }
                
            public:
                constexpr deque(size_t size) noexcept : m_point(), m_ring(size) { }
                ~deque()
                {
                    for (size_t pos{0}; pos < size(); ++pos)
                    {
                        m_ring[pos].~T();
                    }
                }

                template <typename C>
                constexpr bool push_back(C&& arg) noexcept
                {
                    auto tup = get_back_pos();
                    if (std::get<0>(tup) == false)
                    {
                        return false;
                    }
                    else
                    {
                        emplace(std::get<1>(tup), std::forward<C>(arg));
                        m_point.back->store(std::get<1>(tup) + 1, std::memory_order_release);
                        return true;
                    }
                }

                template <typename... Cs>
                constexpr bool emplace_back(Cs&&... args) noexcept
                {
                    auto tup = get_back_pos();
                    if (std::get<0>(tup) == false)
                    {
                        return false;
                    }
                    else
                    {
                        emplace(std::get<1>(tup), std::forward<Cs>(args)...);
                        m_point.back->store(std::get<1>(tup) + 1, std::memory_order_release);
                        return true;
                    }
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                    auto tup = get_front_pos();
                    if (std::get<0>(tup) == false)
                    {
                        return false;
                    }
                    else
                    {
                        std::forward<C>(arg) = std::move(m_ring.operator[](std::get<1>(tup)));
                        m_ring.operator[](std::get<1>(tup)).~T();
                        m_point.front->store(std::get<1>(tup) + 1, std::memory_order_release);
                        return true;
                    }
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
                    false_share<std::atomic<size_t>> m_push_own;
                    false_share<std::atomic<size_t>> m_pop_own;
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

                constexpr bool full(size_t back_pos, size_t front_pos) noexcept { return back_pos >= (front_pos + m_ring.capacity()); }
                constexpr bool empty(size_t back_pos, size_t front_pos) noexcept { return back_pos <= front_pos; }
                constexpr bool full() noexcept { return full(m_point.back->load(std::memory_order_relaxed), m_point.front->load(std::memory_order_relaxed)); }
                constexpr bool empty() noexcept { return empty(m_point.back->load(std::memory_order_relaxed), m_point.front->load(std::memory_order_relaxed)); }
                constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_relaxed) - m_point.front->load(std::memory_order_relaxed); }

                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) { new (m_ring.address(pos)) T(std::forward<Cs>(args)...); }

            public:
                constexpr deque(size_t size) noexcept : m_point(), m_flag(size), m_ring(size) { }
                ~deque() noexcept
                {
                    for (size_t pos{0}; pos < size(); ++pos)
                    {
                        m_ring[pos].~T();
                    }
                }


                template <typename... Cs>
                constexpr bool emplace_back(Cs&&... args) noexcept
                {
                    size_t shadow_pos = m_point.shadow_back->fetch_add(1, std::memory_order_relaxed);
                    size_t shadow_own = m_flag[shadow_pos].m_push_own->fetch_add(1, std::memory_order_relaxed);
                    if (shadow_own >= 1)
                    {
                        m_point.shadow_back->fetch_sub(1, std::memory_order_relaxed);
                        m_flag[shadow_pos].m_push_own->fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        size_t real_pos = m_point.back->fetch_add(1, std::memory_order_relaxed);
                        emplace(real_pos, std::forward<Cs>(args)...);
                        m_flag[real_pos].m_pop_own->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                }

                template <typename C>
                constexpr bool push_back(C&& arg) noexcept
                {
                    size_t shadow_pos = m_point.shadow_back->fetch_add(1, std::memory_order_relaxed);
                    size_t shadow_own = m_flag[shadow_pos].m_push_own->fetch_add(1, std::memory_order_relaxed);
                    if (shadow_own >= 1)
                    {
                        m_point.shadow_back->fetch_sub(1, std::memory_order_relaxed);
                        m_flag[shadow_pos].m_push_own->fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        size_t real_pos = m_point.back->fetch_add(1, std::memory_order_relaxed);
                        emplace(real_pos, std::forward<C>(arg));
                        m_flag[real_pos].m_pop_own->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                    size_t shadow_pos = m_point.shadow_front->fetch_add(1, std::memory_order_relaxed);
                    size_t shadow_own = m_flag[shadow_pos].m_pop_own->fetch_sub(1, std::memory_order_relaxed);
                    if (shadow_own != 1)
                    {
                        m_point.shadow_front->fetch_sub(1, std::memory_order_relaxed);
                        m_flag[shadow_pos].m_pop_own->fetch_add(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        size_t real_pos = m_point.front->fetch_add(1, std::memory_order_relaxed);
                        std::forward<C>(arg) = std::move(m_ring.operator[](real_pos));
                        m_ring.operator[](real_pos).~T();
                        m_flag[real_pos].m_push_own->fetch_sub(1, std::memory_order_relaxed);
                        return true;
                    }
                }
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia  
