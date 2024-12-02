#pragma once

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
            T data;
            tail_data* next;
        };
    } // namespace tail_detail
    
    template <typename T, typename Alloc = std::allocator<T>>
    struct tail
    {
    private:
        using data_t = tail_detail::tail_data<T>;
        using tail_data_alloc = std::allocator_traits<Alloc>::template rebind_alloc<data_t>;
        using allocator_traits_t = std::allocator_traits<tail_data_alloc>;

        compressed_pair<tail_data_alloc, data_t*> compair;
        data_t* saved;

    public:
        constexpr tail(const Alloc& alloc = Alloc()) : compair(compressed_pair_tag::one, alloc, nullptr), saved(nullptr) { }

        ~tail()
        {
            deallocate_proc(compair.second());
            deallocate_proc(saved);
        }

    private:
        constexpr void deallocate_proc(data_t* start_pos)
        {
            if (start_pos == nullptr) { }
            else
            {
                while(start_pos != nullptr)
                {
                    data_t* tmp_pos = start_pos;
                    start_pos = start_pos->next;
                    allocator_traits_t::deallocate(compair.first(), tmp_pos, 1);
                }
            }
        }

    public:
        template <typename Ty>
        constexpr void push_front(Ty&& arg)
        {
            data_t* new_block { };
            if (saved != nullptr) 
            {
                new_block = saved;
                saved = saved->next;
            }
            else
            {
                new_block = allocator_traits_t::allocate(compair.first(), 1);
            }
            allocator_traits_t::construct(compair.first(), new_block, std::forward<Ty>(arg), compair.second());
            compair.second() = new_block;
        }

        constexpr T pop_front()
        {
            data_t* out_block = compair.second();
            compair.second() = out_block->next;
            T ret = std::move(out_block->data);
            allocator_traits_t::destroy(compair.first(), out_block);
            out_block->next = saved;
            saved = out_block;
            return ret;
        }
    };
} // namespace sia
