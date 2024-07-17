#pragma once

#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/container/ring.hpp"
#include "SIA/utility/cache_type.hpp"

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

            template <typename T, typename Allocator = std::allocator<T>>
            struct deque
            {
            private:
                using point_t = deque_detail::point;

                point_t m_point;
                false_share<ring<T, Allocator>> m_ring;

                deque(const deque&) = delete;
                deque& operator=(const deque&) = delete;

                constexpr bool full(size_t back_pos, size_t front_pos) noexcept { return back_pos == (front_pos + m_ring->capacity()); }
                constexpr bool empty(size_t back_pos, size_t front_pos) noexcept { return back_pos == front_pos; }

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
                        arg = std::move(m_ring->operator[](std::get<1>(tup)));
                        m_ring->address(std::get<1>(tup))->~T();
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
            struct deque
            {

            };
        } // namespace mpmc
    } // namespace concurrency
} // namespace sia  
