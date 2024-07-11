#pragma once

#include <type_traits>
#include <memory>

#include "SIA/concurrency/declare.h"
#include "SIA/utility/constant_tag.hpp"
#include "SIA/container/tuple.hpp"

namespace sia
{
    struct thread : private constant_tag<system::os>
    {
    private:
        using tag_t = constant_tag<system::os>;
        void* handle;
        dword_t id;
        template <typename F, typename... Ts>
        constexpr void init(dword_t flag, F&& func, Ts&&... args) noexcept
        {
            if (tag_t::query(tags::os::window))
            {
                using tuple_t = tuple<std::decay_t<F>, std::decay_t<Ts>...>;
                auto decay_copy = std::make_unique<tuple_t>(std::forward<std::decay_t<F>>(func), std::forward<Ts>(args)...);
                LPTHREAD_START_ROUTINE thread_func = reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_detail::get_thread_function<tuple_t>(std::make_index_sequence<sizeof...(Ts) + 1>{ }));
                handle = CreateThread(NULL, NULL, thread_func, decay_copy.get(), flag, reinterpret_cast<LPDWORD>(&id));
                if (handle == nullptr) { terminate(); }
                else { decay_copy.release(); }
            }
            else if (tag_t::query(tags::os::linux))
            {
                handle = pthread_create(flag, func, args...); // testing
            }
            else
            {

            }
        }

    public:
        constexpr thread() noexcept = default;
        constexpr thread(const thread&) noexcept = delete;
        template <typename F, typename... Ts>
        constexpr thread(auto flag, F&& func, Ts&&... args) noexcept
        {
            init(static_cast<dword_t>(flag), std::forward<F>(func), std::forward<Ts>(args)...);
        }
        constexpr thread(thread&& arg) noexcept
        {
            if (joinable()) { terminate(); }
            handle = arg.handle;
            id = arg.id;
            arg.handle = nullptr;
            arg.id = 0;
        }
        ~thread() { if (joinable()) { terminate(); } }

        constexpr thread& operator=(const thread&) noexcept = delete;
        constexpr thread& operator=(thread&& arg) noexcept
        {
            if (joinable()) { terminate(); }
            handle = arg.handle;
            id = arg.id;
            arg.handle = nullptr;
            arg.id = 0;
            return *this;
        }

        constexpr void resume() noexcept
        {
            if (!(handle == nullptr))
            {
                if (tag_t::query(tags::os::window))
                {
                    ResumeThread(handle);
                }
                else if (tag_t::query(tags::os::linux))
                { }
                else
                { }
            }
        }
        constexpr void suspend() noexcept
        {
            if (!(handle == nullptr))
            {
                if (tag_t::query(tags::os::window))
                {
                    SuspendThread(handle);
                }
                else if (tag_t::query(tags::os::linux))
                { }
                else
                { }
            }
        }
        constexpr bool joinable() noexcept { return id != 0; }
        void join() noexcept
        {
            if ((!joinable()) || (id == GetCurrentThreadId())) { terminate(); }
            else
            {
                if (tag_t::query(tags::os::window))
                {
                    dword_t res = WaitForSingleObjectEx(handle, static_cast<dword_t>(thread_tag::time::inf), false);
                    switch (static_cast<thread_tag::wait>(res))
                    {
                        case thread_tag::wait::object_0  :
                            CloseHandle(handle);
                            break;
                        default:
                            terminate();
                    }
                }
                else if (tag_t::query(tags::os::linux))
                { }
                else
                { }
            }
            handle = nullptr;
            id = 0;
        }
        
        void detach() noexcept
        {
            if (!joinable()) { terminate(); }
            else
            {
                if (tag_t::query(tags::os::window))
                {
                    CloseHandle(handle);
                }
                else if (tag_t::query(tags::os::linux))
                { }
                else
                { }
            }
            handle = nullptr;
            id = 0;
        }
    };
} // namespace sia