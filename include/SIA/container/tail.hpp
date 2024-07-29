#pragma once

#include <memory>

#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    template <typename, typename>
    struct tail;

    namespace tail_detail
    {
        template <typename T>
        struct tail_data {
        private:
            tail_data* m_tail;
            T m_data;
        public:
            constexpr tail_data() noexcept : m_tail(nullptr), m_data() { }
            template <typename... Cs>
            constexpr tail_data(tail_data* ptr, Cs&&... args) noexcept : m_tail(ptr), m_data(std::forward<Cs>(args)...) { }
            constexpr tail_data*& tail() noexcept { return m_tail; }
            constexpr T& data() noexcept { return m_data; }
        };
    } // namespace tail_detail

    template <typename T, typename Allocator = std::allocator<T>>
    struct tail {
    private:
        using tail_data_t = tail_detail::tail_data<T>;
        using alloc_rebind = std::allocator_traits<Allocator>::template rebind_alloc<tail_data_t>;
        using allocator_traits_t = std::allocator_traits<alloc_rebind>;

        compressed_pair<alloc_rebind, tail_data_t*> compair;

        template <typename... Cs>
        constexpr tail_data_t* emplace_block(Cs&&... args) noexcept
        {
            tail_data_t* ret = allocator_traits_t::allocate(compair.first(), 1);
            allocator_traits_t::construct(compair.first(), ret, std::forward<Cs>(args)...);
            return ret;
        }
        constexpr tail_data_t* concat(std::initializer_list<tail_data_t*> arg) noexcept
        {
            auto target = arg.begin();
            for(auto iter{arg.begin()}; iter != arg.end(); ++iter)
            {
                if (iter != arg.begin())
                {
                    (*target)->tail() = (*iter);
                    target = iter;
                }
            }
            return *(arg.begin());
        }
    public:
        constexpr tail(const Allocator& alloc = Allocator()) noexcept
            : compair(compressed_pair_tag::one, alloc, nullptr)
        { }

        constexpr tail(std::initializer_list<T> arg, const Allocator& alloc = Allocator()) noexcept
            : compair(compressed_pair_tag::one, alloc, nullptr)
        {
            if (arg.size() > 0)
            {
                tail_data_t* head = emplace_block(nullptr);
                tail_data_t* target = head;
                for (auto iter{arg.begin()}; iter != arg.end(); ++iter)
                {
                    bool is_last = ((arg.end() - 1) == iter);
                    if (is_last)
                    {
                        allocator_traits_t::construct(compair.first(), target, nullptr, *iter);
                    }
                    else
                    {
                        allocator_traits_t::construct(compair.first(), target, allocator_traits_t::allocate(compair.first(), 1), *iter);
                    }
                    target = target->tail();
                }
                compair.second() = head;
            }
        }

        ~tail() noexcept
        {
            tail_data_t* target = compair.second();
            while (target != nullptr)
            {
                tail_data_t* target_next = target_next = target->tail();
                allocator_traits_t::deallocate(compair.first(), target, 1);
                target = target_next;
            }
        }
        constexpr alloc_rebind& get_alloc() noexcept { return compair.first(); }
        constexpr auto& get_compair() noexcept { return compair; }
        constexpr tail_data_t* begin() noexcept { return compair.second(); }

        template <typename... Cs>
        constexpr void insert_after(tail_data_t* ptr, Cs&&... args) noexcept
        {
            tail_data_t* new_data = concat({emplace_block(ptr->tail(), std::forward<Cs>(args))...});
            ptr->tail() = new_data;
        }

        constexpr void remove_after(tail_data_t* ptr) noexcept
        {
            tail_data_t* target = ptr->tail();
            if (target != nullptr) {
                tail_data_t* next = target->tail();
                ptr->tail() = next;
                allocator_traits_t::deallocate(compair.first(), target, 1);
            }
        }
    };
} // namespace sia
