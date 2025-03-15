#pragma once

#include <type_traits>

namespace sia
{
    namespace tags
    {
        enum class quota { take, try_take, have };
    } // namespace tags
    
    namespace quota_detail
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
    } // namespace quota_detail

    template <tags::quota Tag = tags::quota::take, typename T = void>
    struct quota
    {
    private:
        using type = quota;
        T& m_target;
        bool m_own;

        constexpr void in() noexcept(quota_detail::is_in_nothrow<T>())
        {
            if constexpr (quota_detail::LockAble<T>)
            {
                this->m_target.lock();
                this->m_own = true;
            }
            else if constexpr (quota_detail::AcquireAble<T>)
            {
                this->m_target.acquire();
                this->m_own = true;
            }
            else
            { }
        }

        constexpr void try_in() noexcept(quota_detail::is_in_nothrow<T>())
        {
            if constexpr (quota_detail::LockAble<T>)
            { this->m_own = this->m_target.try_lock(); }
            else if constexpr (quota_detail::AcquireAble<T>)
            { this->m_own = this->m_target.try_acquire(); }
            else
            { }
        }
        
        constexpr void out() noexcept(quota_detail::is_out_nothrow<T>())
        {
            if (this->m_own)
            {
                if constexpr (quota_detail::LockAble<T>)
                { this->m_target.unlock(); }
                else if constexpr (quota_detail::AcquireAble<T>)
                { this->m_target.release(); }
                else
                { }
                this->m_own = false;
            }
        }

        template <tags::quota ProcTag = Tag>
        constexpr void proc_init_tag() noexcept(quota_detail::is_in_nothrow<T>())
        {
            if constexpr (ProcTag == tags::quota::take)
            { this->in(); }
            else if constexpr (ProcTag == tags::quota::try_take)
            { this->try_in(); }
            else if constexpr (ProcTag == tags::quota::have)
            { this->m_own = true; }
            else
            { }
        }

        constexpr void proc_init_tag(tags::quota tag) noexcept(quota_detail::is_in_nothrow<T>())
        {
            if (tag == tags::quota::take)
            { this->proc_init_tag<tags::quota::take>(); }
            else if (tag == tags::quota::try_take)
            { this->proc_init_tag<tags::quota::try_take>(); }
            else if (tag == tags::quota::have)
            { this->proc_init_tag<tags::quota::have>(); }
            else
            { }
        }

    public:
        constexpr quota(T& arg, tags::quota tag) noexcept(quota_detail::is_in_nothrow<T>())
            : m_target(arg), m_own()
        { this->proc_init_tag(tag); }

        constexpr quota(T& arg) noexcept(quota_detail::is_in_nothrow<T>())
            : m_target(arg), m_own()
        { this->proc_init_tag(); }

        ~quota() noexcept(quota_detail::is_out_nothrow<T>())
        { this->out(); }

        constexpr bool is_own() noexcept
        { return this->m_own; }

        constexpr bool take() noexcept(quota_detail::is_in_nothrow<T>())
        {
            this->proc_init_tag();
            return this->is_own();
        }

        constexpr bool take(tags::quota tag) noexcept(quota_detail::is_in_nothrow<T>())
        {
            this->proc_init_tag(tag);
            return this->is_own();
        }

        constexpr bool back() noexcept(quota_detail::is_out_nothrow<T>())
        {
            this->out();
            return !this->is_own();
        }
    };

    template <tags::quota Tag = tags::quota::take, typename T = void>
    quota(T&& arg) -> quota<Tag, std::remove_reference_t<T>>;
} // namespace sia
