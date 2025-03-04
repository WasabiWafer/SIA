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
            T* m_data;
        };

        template <typename T>
        struct tail_composition
        {
            tail_data<T>* m_data;
            tail_data<T>* m_buffer;
        };
    } // namespace tail_detail
    
    template <typename T, typename Allocator = std::allocator<T>>
    struct tail
    {
    private:
        using composition_t = tail_detail::tail_composition<T>;
        using tail_t = tail_detail::tail_data<T>;
        using tail_allocator = std::allocator_traits<Allocator>::template rebind_alloc<tail_data_t>;
        using tail_allocator_traits_t = std::allocator_traits<tail_allocator>;
        using data_t = T;
        using data_allocator = Allocator;
        using data_allocator_traits_t = std::allocator_traits<data_allocator>;

        compressed_pair<tail_data_allocator, composition_t> m_compair;

    public:
        constexpr tail(const Allocator& alloc = Allocator())
            : m_compair(splits::one_v, alloc, nullptr, nullptr)
        { }

        ~tail()
        {
            auto& comp = m_compair.second();
            deallocate_proc(comp.m_data);
            deallocate_proc(comp.m_buffer);
        }

    private:
        constexpr tail_data_allocator& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr composition_t& get_composition(this auto&& self) noexcept { return self.m_compair.second(); }
        constexpr void deallocate_proc(tail_data_t* start_pos)
        {
            auto& allocator = this->m_compair.first();
            while(start_pos != nullptr)
            {
                tail_data_t* tmp_pos = start_pos;
                start_pos = start_pos->m_next;
                allocator_traits_t::deallocate(allocator, tmp_pos, 1);
            }
        }

    public:
        constexpr bool is_empty() noexcept
        {
            auto& comp = this->get_composition();
            return comp.m_data == nullptr;
        }

        template <typename... Tys>
        constexpr void emplace_front(Tys&&... args)
        {
            auto& allocator = this->get_allocator();
            auto& comp = this->get_composition();
            data_t* new_block { };
            if (comp.m_buffer != nullptr) 
            {
                new_block = comp.m_buffer;
                comp.m_buffer = comp.m_buffer->m_next;
            }
            else
            {
                new_block = allocator_traits_t::allocate(allocator, 1);
            }
            allocator_traits_t::construct(allocator, new_block, comp.m_data, std::forward<Tys>(args)...);
            comp.m_data = new_block;
        }
        constexpr void push_front(const T& arg) { this->emplace_front(arg); }
        constexpr void push_front(T&& arg)      { this->emplace_front(std::move(arg)); }
        constexpr void pop_front()
        {
            auto& allocator = this->get_allocator();
            auto& comp = this->get_composition();
            if (!this->is_empty())
            {
                data_t* out_block = comp.m_data;
                comp.m_data = comp.m_data->m_next;
                out_block->m_next = comp.m_buffer;
                comp.m_buffer = out_block;
                allocator_traits_t::destroy(allocator, out_block);
            }
        }

        [[nodiscard]]
        constexpr T& front() noexcept
        {
            auto& comp = this->get_composition();
            return comp.m_data->m_data;
        }

        [[nodiscard]]
        constexpr const T& front() const noexcept
        {
            auto& comp = this->get_composition();
            comp.m_data->m_data;
        }
    };
} // namespace sia
