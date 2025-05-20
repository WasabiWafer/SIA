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
        concept QuotaReq = LockAble<T> || AcquireAble<T>;

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

    template <quota_detail::QuotaReq T>
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
            if constexpr (quota_detail::LockAble<T>)
            { this->m_target.unlock(); }
            else if constexpr (quota_detail::AcquireAble<T>)
            { this->m_target.release(); }
            else
            { }
            this->m_own = false;
        }

        constexpr void proc_tag(const tags::quota& tag) noexcept(quota_detail::is_in_nothrow<T>())
        {
            if (tag == tags::quota::take)
            { this->in(); }
            else if (tag == tags::quota::try_take)
            { this->try_in(); }
            else if (tag == tags::quota::have)
            { this->m_own = true; }
            else
            { }
        }

    public:
        constexpr quota(T& arg, tags::quota tag = tags::quota::take) noexcept(quota_detail::is_in_nothrow<T>())
            : m_target(arg), m_own(false)
        { this->proc_tag(tag); }

        ~quota() noexcept(quota_detail::is_out_nothrow<T>())
        { this->back(); }

        constexpr bool is_own() noexcept
        { return this->m_own; }

        constexpr void take() noexcept(quota_detail::is_in_nothrow<T>())
        {
            if (this->is_own())
            { }
            else
            { this->proc_tag(tags::quota::take); }
        }

        constexpr bool try_take() noexcept(quota_detail::is_in_nothrow<T>())
        {
            if (this->is_own())
            { }
            else
            { this->proc_tag(tags::quota::try_take); }
            return this->is_own();
        }

        constexpr void back() noexcept(quota_detail::is_out_nothrow<T>())
        {
            if (this->is_own())
            { this->out(); }
            else
            { }
        }
    };

    template <quota_detail::QuotaReq T>
    quota(T&& arg, tags::quota tag) -> quota<std::remove_reference_t<T>>;
} // namespace sia
