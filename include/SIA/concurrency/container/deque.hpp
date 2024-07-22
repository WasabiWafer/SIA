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
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
