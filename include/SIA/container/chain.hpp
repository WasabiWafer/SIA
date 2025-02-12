#pragma once

#include <utility>
#include <memory>

#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace chain_detail
    {
        template <typename T>
        struct chain_data
        {
            chain_data* m_prev;
            chain_data* m_next;
            T* m_data;
        };

        template <typename T>
        struct chain_composition
        {
            chain_data<T>* m_data;
            chain_data<T>* m_buffer;
            chain_data<T>* m_chain_end;
            T* m_data_end;
        };
    } // namespace chain_detail
    
    template <typename T, size_t Size, typename Allocator = std::allocator<T>>
        requires (Size >= 1)
    struct chain
    {
    private:
        using composition_t = chain_detail::chain_composition<T>;
        using chain_data_t = chain_detail::chain_data<T>;
        using allocator_traits_t = std::allocator_traits<Allocator>;
        using chain_data_allocator = allocator_traits_t::template rebind_alloc<chain_data_t>;
        using allocator_traits_data_t = std::allocator_traits<chain_data_allocator>;
        using chain_data_compair_t = compressed_pair<chain_data_allocator, composition_t>;

        compressed_pair<Allocator, chain_data_compair_t> m_compair;

        constexpr auto& get_composition(this auto&& self) noexcept { return self.m_compair.second().second(); }
        constexpr auto& get_chain_allocator(this auto&& self) noexcept { return self.m_compair.second().first(); }
        constexpr auto& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr chain_data_t* chain_begin(this auto&& self) noexcept { return self.get_composition().m_data; }
        constexpr chain_data_t* chain_end(this auto&& self) noexcept { return self.get_composition().m_chain_end; }
        constexpr T* chain_data_begin(this auto&& self) noexcept { return self.get_composition().m_chain_end->m_data; }
        constexpr T* chain_data_end(this auto&& self) noexcept { return self.get_composition().m_data_end; }
        constexpr size_t chain_capacity(this auto&& self) noexcept { return Size; }
        constexpr size_t chain_size(this auto&& self) noexcept { return self.chain_data_end() - self.chain_data_begin(); }
        constexpr bool is_chain_full(this auto&& self) noexcept { return self.chain_size() == self.chain_capacity(); }
        constexpr bool is_chain_empty(this auto&& self) noexcept { return self.chain_size() == 0; }

        constexpr void proc_add_chain(this auto&& self)
        {
            composition_t& comp = self.get_composition();
            chain_data_t* new_chain { };
            if (comp.m_buffer == nullptr)
            {
                new_chain = allocator_traits_data_t::allocate(self.get_chain_allocator(), 1);
                allocator_traits_data_t::construct(self.get_chain_allocator(), new_chain, nullptr, nullptr, nullptr);
                new_chain->m_data = allocator_traits_t::allocate(self.get_allocator(), self.chain_capacity());
            }
            else
            {
                new_chain = comp.m_buffer;
                comp.m_buffer = comp.m_buffer->m_next;
                new_chain->m_next = nullptr;
            }
            comp.m_chain_end->m_next = new_chain;
            new_chain->m_prev = comp.m_chain_end;
            comp.m_chain_end = new_chain;
            comp.m_data_end = new_chain->m_data;
        }

        constexpr void proc_sub_chain(this auto&& self)
        {
            composition_t& comp = self.get_composition();
            if (comp.m_chain_end->m_prev != nullptr)
            {
                chain_data_t* sub_chain = comp.m_chain_end;
                comp.m_chain_end = sub_chain->m_prev;
                comp.m_data_end = comp.m_chain_end->m_data + self.chain_capacity();
                comp.m_chain_end->m_next = nullptr;
                sub_chain->m_prev = nullptr;
                sub_chain->m_next = comp.m_buffer;
                if (comp.m_buffer != nullptr)
                { comp.m_buffer->m_prev = sub_chain; }
                comp.m_buffer = sub_chain;
            }
        }

        constexpr void proc_dealloc(this auto&& self, chain_data_t* ptr)
        {
            while (ptr != nullptr)
            {
                chain_data_t* next = ptr->m_next;
                allocator_traits_t::deallocate(self.get_allocator(), ptr->m_data, self.chain_capacity());
                allocator_traits_data_t::deallocate(self.get_chain_allocator(), ptr, 1);
                ptr = next;
            }
        }

    public:
        constexpr chain(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, compressed_pair_tag::one, alloc, nullptr, nullptr, nullptr, nullptr)
        {
            composition_t& comp = get_composition();
            comp.m_data = allocator_traits_data_t::allocate(get_chain_allocator(), 1);
            allocator_traits_data_t::construct(get_chain_allocator(), comp.m_data, nullptr, nullptr, nullptr);
            comp.m_data->m_data = allocator_traits_t::allocate(get_allocator(), chain_capacity());
            comp.m_chain_end = comp.m_data;
            comp.m_data_end = comp.m_data->m_data;
        }

        ~chain()
        {
            composition_t& comp = this->get_composition();
            proc_dealloc(comp.m_data);
            proc_dealloc(comp.m_buffer);
        }

        template <typename... Tys>
        constexpr void emplace_back(Tys&&... args)
        {
            if (this->is_chain_full())
            { this->proc_add_chain(); }
            composition_t& comp = get_composition();
            allocator_traits_t::construct(this->get_allocator(), comp.m_data_end, std::forward<Tys>(args)...);
            ++comp.m_data_end;
        }
        constexpr void push_back(const T& arg) { this->emplace_back(arg); }
        constexpr void push_back(T&& arg) { this->emplace_back(std::move(arg)); }

        constexpr void pop_back()
        {
            if (this->is_chain_empty())
            { this-> proc_sub_chain(); }
            if (!this->is_chain_empty())
            {
                composition_t& comp = this->get_composition();
                allocator_traits_t::destroy(this->get_allocator(), --comp.m_data_end);
            }
        }

        [[nodiscard]]
        constexpr T& back(this auto&& self) noexcept { return *self.get_composition().m_data_end; }
    };
} // namespace sia
