#pragma once

#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/align_wrapper.hpp"

// spsc deque
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
                false_share<ring<T>> m_ring;

                deque(const deque&) = delete;
                deque& operator=(const deque&) = delete;

                constexpr bool full(size_t back_pos, size_t front_pos) noexcept { return back_pos >= (front_pos + m_ring->capacity()); }
                constexpr bool empty(size_t back_pos, size_t front_pos) noexcept { return back_pos <= front_pos; }
                constexpr bool full() noexcept { return m_point.back->load(std::memory_order_acquire) >= (m_point.front->load(std::memory_order_acquire) + m_ring->capacity()); }
                constexpr bool empty() noexcept { return m_point.back->load(std::memory_order_acquire) <= m_point.front->load(std::memory_order_acquire); }
                constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_acquire) - m_point.front->load(std::memory_order_acquire); }

                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) { new (m_ring->address(pos)) T(std::forward<Cs>(args)...); }

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
                constexpr deque() noexcept = default;
                constexpr deque(size_t size) noexcept : m_point(), m_ring(size) { }
                ~deque()
                {
                    while(true)
                    {
                        auto tup = get_front_pos();
                        if (std::get<0>(tup) == false)
                        {
                            return ;
                        }
                        else
                        {
                            m_ring->address(std::get<1>(tup))->~T();
                            m_point.front->store(std::get<1>(tup) + 1, std::memory_order_release);
                        }
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
                        std::forward<C>(arg) = std::move(m_ring->operator[](std::get<1>(tup)));
                        // m_ring->address(std::get<1>(tup))->~T();
                        m_ring->operator[](std::get<1>(tup)).~T();
                        m_point.front->store(std::get<1>(tup) + 1, std::memory_order_release);
                        return true;
                    }
                }
            };
        } // namespace spsc
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
                struct point
                {
                    false_share<std::atomic<size_t>> back;
                    false_share<std::atomic<size_t>> front;
                    false_share<std::atomic<size_t>> shadow_back;
                    false_share<std::atomic<size_t>> shadow_front;
                };

                struct flag_composit
                {
                    false_share<std::atomic<size_t>> m_own;
                    false_share<std::atomic<size_t>> m_init;
                };
            } // namespace deque_detail
            
            template <typename T>
            struct deque
            {
                using flag_t = deque_detail::flag_composit;
                using point_t = deque_detail::point;

                point_t m_point;
                false_share<ring<flag_t>> m_flag;
                false_share<ring<T>> m_ring;

                constexpr deque(size_t size) noexcept : m_point(), m_flag(size), m_ring(size) { }
                template <typename... Cs>
                constexpr void emplace(size_t pos, Cs&&... args) { new (m_ring->address(pos)) T(std::forward<Cs>(args)...); }

                constexpr bool full(size_t back_pos, size_t front_pos) noexcept { return back_pos >= (front_pos + m_ring->capacity()); }
                constexpr bool empty(size_t back_pos, size_t front_pos) noexcept { return back_pos <= front_pos; }
                constexpr bool full() noexcept { return m_point.back->load(std::memory_order_acquire) >= (m_point.front->load(std::memory_order_acquire) + m_ring->capacity()); }
                constexpr bool empty() noexcept { return m_point.back->load(std::memory_order_acquire) <= m_point.front->load(std::memory_order_acquire); }
                constexpr size_t size() noexcept { return m_point.back->load(std::memory_order_acquire) - m_point.front->load(std::memory_order_acquire); }

                template <typename C>
                constexpr bool push_back(C&& arg) noexcept
                {
                    size_t pos = m_point.shadow_back->fetch_add(1, std::memory_order_relaxed);
                    size_t own = m_flag->operator[](pos).m_own->fetch_add(1, std::memory_order_relaxed);
                    if (own >= 1 || full() || (pos != m_point.back->load(std::memory_order_relaxed)))
                    {
                        m_point.shadow_back->fetch_sub(1, std::memory_order_relaxed);
                        m_flag->operator[](pos).m_own->fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        emplace(pos, std::forward<C>(arg));
                        m_flag->operator[](pos).m_init->fetch_add(1, std::memory_order_relaxed);
                        m_point.back->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                }

                template <typename... Cs>
                constexpr bool emplace_back(Cs&&... args) noexcept
                {
                    size_t pos = m_point.shadow_back->fetch_add(1, std::memory_order_relaxed);
                    size_t own = m_flag->operator[](pos).m_own->fetch_add(1, std::memory_order_relaxed);
                    if (own >= 1 || full() || (pos != m_point.back->load(std::memory_order_relaxed)))
                    {
                        m_point.shadow_back->fetch_sub(1, std::memory_order_relaxed);
                        m_flag->operator[](pos).m_own->fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        emplace(pos, std::forward<Cs>(args)...);
                        m_point.back->fetch_add(1, std::memory_order_relaxed);
                        m_flag->operator[](pos).m_init->fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                }

                template <typename C>
                constexpr bool pop_front(C&& arg) noexcept
                {
                    size_t pos = m_point.shadow_front->fetch_add(1, std::memory_order_relaxed);
                    size_t init = m_flag->operator[](pos).m_init->load(std::memory_order_relaxed);
                    if (init == 0 || empty() || (pos != m_point.front->load(std::memory_order_relaxed)))
                    {
                        m_point.shadow_front->fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
                    else
                    {
                        std::forward<C>(arg) = std::move(m_ring->operator[](pos));
                        m_ring->operator[](pos).~T();
                        m_point.front->fetch_add(1, std::memory_order_relaxed);
                        m_flag->operator[](pos).m_init->fetch_sub(1, std::memory_order_release);
                        m_flag->operator[](pos).m_own->fetch_sub(1, std::memory_order_release);
                        return true;
                    }
                }
            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia  
