#pragma once

#include <type_traits>

namespace sia
{
    namespace guard_detail
    {
        template <typename T>
        concept LockAble = requires (T arg) { arg.lock(); arg.unlock(); };
        template <typename T>
        concept AcquireAble = requires (T arg) { arg.acquire(); arg.release(); };

        template <typename T>
        consteval bool is_in_nothrow() noexcept
        {
            if constexpr (LockAble<T>)
            { return noexcept(T().lock()); }
            else if constexpr (AcquireAble<T>)
            { return noexcept(T().acquire()); }
            else
            { return false; }
        }

        template <typename T>
        consteval bool is_out_nothrow() noexcept
        {
            if constexpr (LockAble<T>)
            { return noexcept(T().unlock()); }
            else if constexpr (AcquireAble<T>)
            { return noexcept(T().release()); }
            else
            { return false; }
        }
    } // namespace guard_detail


    template <typename T>
    struct guard
    {
    private:
        using type = guard;
        T& m_target;


        constexpr void in() noexcept(guard_detail::is_in_nothrow<T>())
        {
            if constexpr (guard_detail::LockAble<T>)
            { m_target.lock(); }
            else if constexpr (guard_detail::AcquireAble<T>)
            { m_target.acquire(); }
            else
            { }
        }
        constexpr void out() noexcept(guard_detail::is_out_nothrow<T>())
        {
            if constexpr (guard_detail::LockAble<T>)
            { m_target.unlock(); }
            else if constexpr (guard_detail::AcquireAble<T>)
            { m_target.release(); }
            else
            { }
        }

    public:
        constexpr guard(T& arg) noexcept(noexcept(this->in()))
            : m_target(arg)
        { this->in(); }
        constexpr guard(T&& arg) noexcept(noexcept(this->in()))
            : m_target(arg)
        { this->in(); }
        ~guard() noexcept(noexcept(this->out()))
        { this->out(); }
    };

    template <typename T>
    guard(T&& arg) -> guard<std::remove_reference_t<T>>;
} // namespace sia
