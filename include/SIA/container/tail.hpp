#pragma once

#include <utility>
#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace tail_detail
    {
        template <typename T>
        struct tail_data
        {
            tail_data* m_next;
            T m_data;

            template <typename... Tys>
            constexpr tail_data(tail_data* next, Tys&&... args)
                : m_next(next), m_data(std::forward<Tys>(args)...)
            { }
        };
    } // namespace tail_detail
    
    template <typename T, typename Allocator = std::allocator<T>>
    struct tail
    {
    private:
        using data_t = tail_detail::tail_data<T>;
        using tail_data_alloc = std::allocator_traits<Allocator>::template rebind_alloc<data_t>;
        using allocator_traits_t = std::allocator_traits<tail_data_alloc>;

        compressed_pair<tail_data_alloc, data_t*> m_compair;
        data_t* m_saved;

    public:
        constexpr tail(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr), m_saved(nullptr)
        { }

        ~tail()
        {
            deallocate_proc(m_compair.second());
            deallocate_proc(m_saved);
        }

    private:
        constexpr void deallocate_proc(data_t* start_pos)
        {
            while(start_pos != nullptr)
            {
                data_t* tmp_pos = start_pos;
                start_pos = start_pos->m_next;
                allocator_traits_t::deallocate(m_compair.first(), tmp_pos, 1);
            }
        }

    public:
        template <typename... Tys>
        constexpr void emplace_front(Tys&&... args)
        {
            data_t* new_block { };
            if (m_saved != nullptr) 
            {
                new_block = m_saved;
                m_saved = m_saved->m_next;
            }
            else
            {
                new_block = allocator_traits_t::allocate(m_compair.first(), 1);
            }
            allocator_traits_t::construct(m_compair.first(), new_block, m_compair.second(), std::forward<Tys>(args)...);
            m_compair.second() = new_block;
        }
        constexpr void push_front(const T& arg) { this->emplace_front(arg); }
        constexpr void push_front(T&& arg) { this->emplace_front(std::move(arg)); }
        
        template <typename Ty>
        constexpr bool try_pop_front(Ty&& ret)
        {
            if (m_compair.second() == nullptr) { return false; }
            else
            {
                data_t* out_block = m_compair.second();
                m_compair.second() = out_block->m_next;
                out_block->m_next = m_saved;
                m_saved = out_block;
                ret = std::move(out_block->m_data);
                return true;
            }
        }
    };
} // namespace sia
