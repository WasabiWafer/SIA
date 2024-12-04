#pragma once

#include <memory>

#include "SIA/container/lane.hpp"

namespace sia
{
    namespace chain_detail
    {
        template <typename T, size_t Size>
        struct chain_data
        {
            chain_data* m_prev;
            chain_data* m_next;
            T m_data[Size];
        };
    } // namespace chain_detail
    
    template <typename T, size_t Size = 1, typename Allocator = std::allocator<T>>
        requires (Size >= 1)
    struct chain
    {
    private:
        using data_t = chain_detail::chain_data<T, Size>;
        using chain_allocator_t = std::allocator_traits<Allocator>::template rebind_alloc<data_t>;
        using allocator_traits_t = std::allocator_traits<chain_allocator_t>;

        compressed_pair<chain_allocator_t, data_t*> m_compair;
        data_t* m_end_point;
        T* m_data_end_point;

        void add_chain()
        {
            if (m_end_point->m_next == nullptr)
            {
                data_t* prev = m_end_point;
                data_t* next = allocator_traits_t::allocate(m_compair.first(), 1);
                allocator_traits_t::construct(m_compair.first(), next, nullptr, nullptr);
                m_end_point->m_next = next;
                next->m_prev = prev;
                m_end_point = next;
            }
            else
            {
                m_end_point = m_end_point->m_next;
            }
            m_data_end_point = m_end_point->m_data;
        }

        bool chain_back() noexcept
        {
            if (m_end_point->m_prev == nullptr) { return false; }
            else
            {
                m_end_point = m_end_point->m_prev;
                m_data_end_point = (m_end_point->m_data + Size);
                return true;
            }
        }

        template <typename Ty>
        void chain_pop_back(Ty&& arg) noexcept
        {
            arg = std::move(*(--m_data_end_point));
        }
        constexpr size_t chain_capacity() noexcept { return Size; }
        constexpr size_t chain_size() noexcept { return m_data_end_point - m_end_point->m_data; }
        constexpr size_t is_chain_full() noexcept { return chain_size() == chain_capacity(); }
        constexpr size_t is_chain_empty() noexcept { return chain_size() == 0; }

    public:
        constexpr chain(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr), m_end_point(nullptr), m_data_end_point(nullptr)
        {
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), 1);
            allocator_traits_t::construct(m_compair.first(), m_compair.second(), nullptr, nullptr);
            m_end_point = m_compair.second();
            m_data_end_point = m_compair.second()->m_data;
        }

        template <typename Ty>
        constexpr void push_back(Ty&& arg)
        {
            if (is_chain_full()) { add_chain(); }
            *m_data_end_point = std::forward<Ty>(arg);
            ++m_data_end_point;
        }

        template <typename... Tys>
        constexpr void emplace_back(Tys&&... args)
        {
            if (is_chain_full()) { add_chain(); }
            allocator_traits_t::construct(m_compair.first(), m_data_end_point, std::forward<Tys>(args)...);
            ++m_data_end_point;
        }

        template <typename Ty>
        constexpr bool try_pop_back(Ty&& arg) noexcept
        {
            if (is_chain_empty())
            {
                if(chain_back())
                {
                    chain_pop_back(std::forward<Ty>(arg));
                    return true;
                }
            }
            else
            {
                chain_pop_back(std::forward<Ty>(arg));
                return true;
            }
            return false;
        }
    };
} // namespace sia
