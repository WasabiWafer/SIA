#pragma once

#include <type_traits>
#include <atomic>

namespace sia
{
    namespace tags
    {
        enum class quota { take, try_take, have };
    } // namespace tags

    namespace quota_detail
    {
        template <typename T>
        concept LockAble = requires (T arg) { arg.lock(); arg.unlock(); arg.try_lock(); };
        template <typename T>
        concept AcquireAble = requires (T arg) { arg.acquire(); arg.release(); arg.try_acquire(); };
        template <typename T>
        concept QuotaAble = LockAble<T> || AcquireAble<T>;

        template <typename T>
        struct quota_base;

        template <LockAble T>
        struct quota_base<T>
        {
            template <typename>
            friend class quota;
            T& m_target;
            constexpr bool try_take(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { return m_target.try_lock(mem_order); }
            constexpr void take(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { m_target.lock(mem_order); }
            constexpr void back(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { m_target.unlock(mem_order); }
            constexpr bool is_own(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            { return m_target.is_own(mem_order); }
        };

        template <AcquireAble T>
        struct quota_base<T>
        {
            template <typename>
            friend class quota;
            T& m_target;
            bool m_own;
            constexpr bool try_take(std::memory_order rmw_mem_order = std::memory_order::seq_cst, std::memory_order load_mem_order = std::memory_order::seq_cst) noexcept
            { return m_own = m_target.try_acquire(rmw_mem_order, load_mem_order); }
            constexpr void take(std::memory_order rmw_mem_order = std::memory_order::seq_cst, std::memory_order load_mem_order = std::memory_order::seq_cst) noexcept
            {
                m_target.acquire(rmw_mem_order, load_mem_order);
                m_own = true;
            }
            constexpr void back(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
            {
                m_target.release(mem_order);
                m_own = false;
            }
            constexpr bool is_own() noexcept
            { return m_own; }
        };
    } // namespace quota_detail

    template <quota_detail::QuotaAble T>
    struct quota : private quota_detail::quota_base<T>
    {
        private:
            using base_type = quota_detail::quota_base<T>;
        public:
            constexpr quota(auto& arg, tags::quota qtag = tags::quota::take) noexcept
                : base_type(arg)
            {
                if (qtag == tags::quota::take)
                { base_type::take(); }
                else if (qtag == tags::quota::try_take)
                { base_type::try_take(); }
                else if (qtag == tags::quota::have)
                {
                    if constexpr (quota_detail::AcquireAble<T>)
                    { base_type::m_own = true; }
                }
            }
            constexpr ~quota() noexcept
            {
                if (base_type::is_own())
                { base_type::back(); }
            }
    };

    template <typename T>
    quota(T&& arg) -> quota<std::remove_reference_t<T>>;
} // namespace sia
