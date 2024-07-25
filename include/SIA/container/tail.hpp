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
            template <typename... Cs>
            constexpr tail_data(tail_data* ptr, Cs&&... args) noexcept : m_tail(ptr), m_data(std::forward<Cs>(args)...) { }
            constexpr tail_data*& tail() noexcept { return m_tail; }
            constexpr T& data() noexcept { return m_data; }
        };
    } // namespace tail_detail

    template <typename T> using tail_data_t = tail_detail::tail_data<T>;
    template <typename T, typename Allocator = std::allocator<tail_data_t<T>>>
    struct tail {
    private:
        using tail_data_t = tail_detail::tail_data<T>;
        using allocator_traits_t = std::allocator_traits<Allocator>;

        compressed_pair<Allocator, tail_data_t*> compair;

        template <typename... Cs>
        constexpr void emplace(tail_data_t* at, tail_data_t* ptr, Cs&&... args) noexcept { allocator_traits_t::construct(compair.first(), at, ptr, std::forward<Cs>(args)...); }
        template <typename... Cs>
        constexpr tail_data_t* gen_block(tail_data_t* ptr, Cs&&... args) noexcept
        {
            tail_data_t* ret = allocator_traits_t::allocate(compair.first(), 1);
            allocator_traits_t::construct(compair.first(), ret, ptr, std::forward<Cs>(args)...);
            return ret;
        }
        constexpr tail_data_t* concat(std::initializer_list<tail_data_t*> arg) noexcept
        {
            auto tmp = arg.begin();
            for(auto iter{arg.begin() + 1}; iter != arg.end(); ++iter)
            {
                (*tmp)->tail() = (*iter);
                tmp = iter;
            }
            return *(arg.begin());
        }
    public:
        ~tail() noexcept
        {
            tail_data_t* target = compair.second();
            tail_data_t* target_next { };
            while (target != nullptr)
            {
                target_next = target->tail();
                allocator_traits_t::deallocate(compair.first(), target, 1);
                target = target_next;
            }
        }
        constexpr tail() noexcept : compair(compressed_pair_tag::zero, nullptr) { }
        template <typename C, typename... Cs>
        constexpr tail(C&& arg, Cs&&... args) noexcept : compair(compressed_pair_tag::zero, gen_block(nullptr, std::forward<C>(arg)))
        {
            compair.second()->tail() = concat({gen_block(nullptr, std::forward<Cs>(args))...});
        }

        constexpr tail_data_t* begin() noexcept { return compair.second(); }
        template <typename... Cs>
        constexpr void insert_after(tail_data_t* ptr, Cs&&... args) noexcept
        {
            tail_data_t* new_data = concat({gen_block(ptr->tail(), std::forward<Cs>(args))...});
            ptr->tail() = new_data;
        }

        constexpr void remove_after(tail_data_t* ptr) noexcept
        {
            tail_data_t* target = ptr->tail();
            if (target == nullptr) { }
            else
            {
                tail_data_t* next = target->tail();
                ptr->tail() = next;
                allocator_traits_t::deallocate(compair.first(), target, 1);
            }
        }
    };
} // namespace sia
