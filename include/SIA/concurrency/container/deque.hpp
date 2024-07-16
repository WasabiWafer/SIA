#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <type_traits>

#include "SIA/internals/types.hpp"
#include "SIA/utility/constant_tag.hpp"
#include "SIA/concurrency/atomic/mutex.hpp"

// SPSC deque
namespace sia
{
    namespace concurrency
    {
        template <typename, typename>
        struct deque;
    } // namespace concurrency
    

    namespace concurrency_deque_detail
    {
        template <typename, typename>
        struct deque;

        enum class action { push, pop };

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
            explicit ring(size_t size) noexcept : capacity(size), data(this->Allocator::allocate(size)) { }
            ~ring() { this->Allocator::deallocate(data, capacity); }
            T* operator[](size_t pos) noexcept { return data + (pos%capacity); }
        };

        template <auto Tag, typename T, typename Allocator>
        struct place : private constant_tag<Tag>
        {
        private:
            using tag_t = constant_tag<Tag>;
            using deque_t = concurrency::deque<T, Allocator>;

            deque_t* que;
            size_t cursor;

            place(const place&) noexcept = delete;
            place& operator=(const place&) = delete;

        public:
            place(deque_t* arg, size_t cur) noexcept : que(arg), cursor(cur) { }
            place() noexcept = default;

            ~place()
            {
                if (que != nullptr)
                {
                    if (this->tag_t::query(action::push))
                    {
                        que->cursor.push.store(cursor + 1, std::memory_order_release);
                    }
                    else if (this->tag_t::query(action::pop))
                    {
                        this->get()->~T();
                        que->cursor.pop.store(cursor + 1, std::memory_order_release);
                    }
                }
            }

            T* get() noexcept
            {
                if (que != nullptr) { return que->operator[](cursor); }
                return nullptr;
            }
            void release() noexcept { que = nullptr; }
            operator bool() { return (que != nullptr); }

        };
    } // namespace concurrency_deque_detail
    
    namespace concurrency
    {
        template <typename T, typename Allocator = std::allocator<T>>
        struct deque
        {
        private:
            template <auto, typename, typename>
            friend struct concurrency_deque_detail::place;

            template <auto E, typename T0, typename T1>
            using place_t = concurrency_deque_detail::place<E, T0, T1>;
            using push_t = place_t<concurrency_deque_detail::action::push, T, Allocator>;
            using pop_t = place_t<concurrency_deque_detail::action::pop, T, Allocator>;

            concurrency_deque_detail::cursor cursor;
            concurrency_deque_detail::ring<T, Allocator> ring;

            deque() = delete;
            deque(const deque&) = delete;
            deque& operator=(const deque&) = delete;

            bool full(size_t c_push, size_t c_pop) noexcept
            {
                if (c_push >= c_pop) { return (c_push - c_pop) == ring.capacity; }
                else { return (c_pop - c_push) == ring.capacity; }
            }
            bool empty(size_t c_push, size_t c_pop) noexcept { return c_push == c_pop; }

            push_t push() noexcept
            {
                size_t c_push = cursor.push.load(std::memory_order_relaxed);
                if (full(c_push, cursor.cache_pop))
                {
                    cursor.cache_pop = cursor.pop.load(std::memory_order_acquire);
                    if (full(c_push, cursor.cache_pop)) { return { }; }
                }
                return {this, c_push};
            }

            pop_t pop() noexcept
            {
                size_t c_pop = cursor.pop.load(std::memory_order_relaxed);
                if (empty(cursor.cache_push, c_pop))
                {
                    cursor.cache_push = cursor.push.load(std::memory_order_acquire);
                    if (empty(cursor.cache_push, c_pop)) { return pop_t{ }; }
                }
                return {this, c_pop};
            }

        public:
            deque(size_t size) noexcept : cursor(), ring(size) { }

            T* operator[](size_t pos) noexcept { return ring[pos]; }

            size_t capacity() noexcept { return ring.capacity; }
            size_t size() noexcept { return cursor.push_cursor - cursor.pop_cursor; }
            bool empty() noexcept { return size() == 0; }
            bool full() noexcept { return size() == ring.capacity; }
            
            template <typename C>
            bool push(C&& arg) noexcept
            {
                push_t place = push();
                if (place)
                {
                    new (place.get()) C{std::forward<C>(arg)};
                    return true;
                }
                return false;
            }

            template <typename C>
            bool pop(C&& arg) noexcept
            {
                pop_t place = pop();
                if (place)
                {
                    arg = std::move(*(place.get()));
                    return true;
                }
                return false;
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
