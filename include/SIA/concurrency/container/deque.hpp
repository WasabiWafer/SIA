#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

#include "SIA/internals/types.hpp"
#include "SIA/concurrency/atomic/mutex.hpp"

// SPSC deque
namespace sia
{
    namespace concurrency_deque_detail
    {
        struct cursor
        {
        public:
            static_assert(std::atomic<size_t>::is_always_lock_free);
            alignas(std::hardware_destructive_interference_size) std::atomic<size_t> push;
            alignas(std::hardware_destructive_interference_size) size_t cache_push;
            alignas(std::hardware_destructive_interference_size) std::atomic<size_t> pop;
            alignas(std::hardware_destructive_interference_size) size_t cache_pop;
        private:
            unsigned_interger_t<1> padding[std::hardware_destructive_interference_size - sizeof(size_t)];
        };

        template <typename T, typename Allocator = std::allocator<T>>
        struct ring : private Allocator
        {
            size_t capacity;
            T* data;
            explicit ring(size_t size) : capacity(size), data(this->Allocator::allocate(size)) { }
            ~ring() { this->Allocator::deallocate(data, capacity); }
            T* operator[](size_t pos) noexcept { return data + (pos%capacity); }
        };
    } // namespace concurrency_deque_detail
    
    namespace concurrency
    {
        template <typename T, typename Allocator = std::allocator<T>>
        struct deque
        {
        private:
            concurrency_deque_detail::cursor cursor;
            concurrency_deque_detail::ring<T, Allocator> ring;
            
            deque() = delete;
            deque(const deque&) = delete;
            deque& operator=(const deque&) = delete;

            bool full(size_t c_push, size_t c_pop) noexcept { return (c_push - c_pop) == ring.capacity; }
            bool empty(size_t c_push, size_t c_pop) noexcept { return c_push == c_pop; }
        public:
            explicit deque(size_t size) : cursor(), ring(size) { }

            size_t capacity() noexcept { return ring.capacity; }
            size_t size() noexcept { return cursor.push_cursor - cursor.pop_cursor; }
            bool empty() noexcept { return size() == 0; }
            bool full() noexcept { return size() == ring.capacity; }
            
            template <typename C>
            bool push(C&& arg) noexcept
            {
                size_t c_push = cursor.push.load(std::memory_order_relaxed);
                if (full(c_push, cursor.cache_pop))
                {
                    cursor.cache_pop = cursor.pop.load(std::memory_order_acquire);
                    if (full(c_push, cursor.cache_pop)) { return false; }
                }
                new (ring[c_push]) T(std::forward<C>(arg));
                cursor.push.store(c_push + 1, std::memory_order_release);
                return true;
            }

            template <typename C>
            bool pop(C&& arg) noexcept
            {
                size_t c_pop = cursor.pop.load(std::memory_order_relaxed);
                if (empty(cursor.cache_push, c_pop))
                {
                    cursor.cache_push = cursor.push.load(std::memory_order_acquire);
                    if (empty(cursor.cache_push, c_pop)) { return false; }
                }
                arg = std::move(*(ring[c_pop]));
                ring[c_pop]->~T();
                cursor.pop.store(c_pop + 1, std::memory_order_release);
                return true;
            }
        };

        template <typename T, typename Allocator>
        struct deque_service
        {
        private:
            deque<T, Allocator>& que;
            mutex mtx_push;
            mutex mtx_pop;

            deque_service() = delete;
            deque_service(const deque_service&) = delete;
            deque_service& operator=(const deque_service&) = delete;

        public:
            explicit deque_service(deque<T, Allocator>& arg) : que(arg), mtx_push(), mtx_pop() { }

            template <typename C>
            bool push(C&& arg) noexcept
            {
                std::lock_guard lg{mtx_push};
                return que.push(std::forward<C>(arg));
            }

            template <typename C>
            bool pop(C&& arg) noexcept
            {
                std::lock_guard lg{mtx_pop};
                return que.pop(std::forward<C>(arg));
            }
        };
    } // namespace concurrency
} // namespace sia
