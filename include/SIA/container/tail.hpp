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
            T m_data[1];
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
        using tail_data_t = tail_detail::tail_data<T>;
        using tail_allocator_t = std::allocator_traits<Allocator>::template rebind_alloc<tail_data_t>;
        using tail_allocator_traits_t = std::allocator_traits<tail_allocator_t>;

        compressed_pair<tail_allocator_t, composition_t> m_compair;

    public:
        constexpr tail(const tail_allocator_t& alloc = Allocator()) noexcept(noexcept(tail_allocator_t()))
            : m_compair(splits::one_v, alloc, nullptr, nullptr)
        { }

        ~tail() noexcept(noexcept(deallocate_proc(m_compair.second().m_data)) && noexcept(T().~T()))
        {
            auto& comp = m_compair.second();
            deallocate_proc(comp.m_data);
            deallocate_proc(comp.m_buffer);
        }

    private:
        constexpr tail_allocator_t& get_tail_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr composition_t& get_composition(this auto&& self) noexcept { return self.m_compair.second(); }
        constexpr void deallocate_proc(this auto&& self, tail_data_t* ptr) noexcept(noexcept(tail_allocator_traits_t::deallocate(self.get_tail_allocator(), ptr, 1)) && noexcept(T().~T()))
        {
            tail_allocator_t& t_alloc = self.get_tail_allocator();
            while(ptr != nullptr)
            {
                tail_data_t* next = ptr->m_next;
                tail_allocator_traits_t::deallocate(t_alloc, ptr, 1);
                ptr = next;
            }
        }

    public:
        constexpr bool is_empty(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_data == nullptr;
        }

        template <typename... Tys>
        constexpr void emplace_front(Tys&&... args) noexcept(noexcept(tail_allocator_traits_t::allocate(this->get_tail_allocator(), 1)) && noexcept(T(std::forward<Tys>(args)...)))
        {
            tail_allocator_t& t_alloc = this->get_tail_allocator();
            composition_t& comp = this->get_composition();

            tail_data_t* new_block;

            if (comp.m_buffer != nullptr) 
            {
                new_block = comp.m_buffer;
                comp.m_buffer = comp.m_buffer->m_next;
            }
            else
            {
                new_block = tail_allocator_traits_t::allocate(t_alloc, 1);
            }

            std::construct_at(new_block->m_data, std::forward<Tys>(args)...);
            new_block->m_next = comp.m_data;
            comp.m_data = new_block;
        }
        constexpr void push_front(const T& arg) noexcept(noexcept(this->emplace_front(arg))) { this->emplace_front(arg); }
        constexpr void push_front(T&& arg) noexcept(noexcept(this->emplace_front(std::move(arg)))) { this->emplace_front(std::move(arg)); }

        constexpr void pop_front() noexcept(noexcept(T().~T()))
        {
            tail_allocator_t& t_alloc = this->get_tail_allocator();
            composition_t& comp = this->get_composition();
            if (!this->is_empty())
            {
                tail_data_t* out = comp.m_data;
                comp.m_data = comp.m_data->m_next;
                out->m_next = comp.m_buffer;
                comp.m_buffer = out;
                std::destroy_at(out->m_data);
            }
        }

        [[nodiscard]]
        constexpr T& front() noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_data->m_data);
        }

        [[nodiscard]]
        constexpr const T& front() const noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_data->m_data);
        }
    };
} // namespace sia
