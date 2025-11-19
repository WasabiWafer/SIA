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
        concept CheckAble = requires (T arg) { arg.check_in(); arg.check(0); arg.check_out(); };
        template <typename T>
        concept QuotaAble = LockAble<T> || AcquireAble<T> || CheckAble<T>;

        template <typename T>
        struct quota_base;

        template <LockAble T>
        struct quota_base<T>
        {
                template <typename>
                friend class quota;
            private:
                T& m_target;
            public:
                constexpr quota_base(T& arg) noexcept : m_target(arg) { }
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
            private:
                T& m_target;
            protected:
                bool m_own;
            public:
                constexpr quota_base(T& arg, bool flag) noexcept : m_target(arg), m_own(flag) { }
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
                constexpr bool is_own() noexcept { return m_own; }
        };

        template <CheckAble T>
        struct quota_base<T>
        {
                template <typename>
                friend class quota;
            private:
                using value_type = T::value_type;
                T& m_target;
                value_type m_hold;
            protected:
                bool m_own;
            public:
                constexpr quota_base(T& arg, bool flag) noexcept : m_target(arg), m_own(flag), m_hold() { }

                constexpr void wait(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
                { while (!m_target.check(m_hold, mem_order)) { } }
                constexpr void set_number(value_type arg) noexcept { m_hold = arg; }
                constexpr bool try_take(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
                {
                    m_hold = m_target.check_in(mem_order);
                    m_own = true;
                    return true;
                }
                constexpr void take(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
                {
                    try_take(mem_order);
                    wait();
                }
                constexpr void back(std::memory_order mem_order = std::memory_order::seq_cst) noexcept
                {
                    m_target.check_out(mem_order);
                    m_own = false;
                }
                constexpr bool is_own() noexcept { return m_own; }
        };
    } // namespace quota_detail

    template <quota_detail::QuotaAble T>
    struct quota : public quota_detail::quota_base<T>
    {
        private:
            using base_type = quota_detail::quota_base<T>;

            template <typename Ty = size_t>
            constexpr void init(tags::quota qtag, Ty arg = 0) noexcept
            {
                if (qtag == tags::quota::take)
                { this->base_type::take(); }
                else if (qtag == tags::quota::try_take)
                { this->base_type::try_take(); }
                else if (qtag == tags::quota::have)
                {
                    if constexpr (quota_detail::AcquireAble<T>)
                    { this->base_type::m_own = true; }
                    else if constexpr (quota_detail::CheckAble<T>)
                    {
                        this->base_type::set_number(arg);
                        this->base_type::m_own = true;
                    }
                }
            }

        public:
            template <quota_detail::LockAble Ty>
            constexpr quota(Ty&& arg, tags::quota qtag = tags::quota::take) noexcept
                : base_type(arg)
            { init(qtag); }

            template <quota_detail::AcquireAble Ty>
            constexpr quota(Ty&& arg, tags::quota qtag = tags::quota::take) noexcept
                : base_type(arg, false)
            { init(qtag); }

            template <quota_detail::CheckAble Ty>
            constexpr quota(Ty&& arg, tags::quota qtag = tags::quota::take) noexcept
                : base_type(arg, false)
            { init(qtag); }

            constexpr ~quota() noexcept
            {
                if (this->base_type::is_own())
                { this->base_type::back(); }
            }
    };

    template <typename T>
    quota(T&& arg, tags::quota qtag = tags::quota::take) -> quota<std::remove_reference_t<T>>;
} // namespace sia
