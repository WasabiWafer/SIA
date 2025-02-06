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
        using chain_data_allocator = std::allocator_traits<Allocator>::template rebind_alloc<chain_data_t>;
        using inner_compair = compressed_pair<chain_data_allocator, composition_t>;
        using allocator_traits_t = std::allocator_traits<Allocator>;
        using allocator_traits_data_t = std::allocator_traits<chain_data_allocator>;

        compressed_pair<Allocator, inner_compair> m_compair;

        constexpr auto& get_comp(this auto&& self) noexcept { return self.m_compair.second().second(); }
        constexpr auto& get_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr auto& get_data_allocator(this auto&& self) noexcept { return self.m_compair.second().first(); }
        constexpr size_t get_chain_capacity(this auto&& self) noexcept { return Size; }
        constexpr chain_data_t* chain_begin(this auto&& self) noexcept { return self.get_comp().m_data; }
        constexpr chain_data_t* chain_end(this auto&& self) noexcept { return self.get_comp().m_chain_end; }
        constexpr T* data_end_begin(this auto&& self) noexcept { return self.chain_end()->m_data; }
        constexpr T* data_end_end(this auto&& self) noexcept { return self.get_comp().m_data_end; }
        constexpr size_t get_chain_data_size(this auto&& self) noexcept { return self.data_end_end() - self.data_end_begin(); }
        constexpr bool is_chain_data_full(this auto&& self) noexcept { return self.get_chain_capacity() == self.get_chain_data_size(); }
        constexpr bool is_chain_data_empty(this auto&& self) noexcept { return 0 == self.get_chain_data_size(); }

        constexpr void proc_chain_dealloc(this auto&& self, chain_data_t* ptr)
        {
            while(ptr != nullptr)
            {
                chain_data_t* next = ptr->m_next;
                allocator_traits_t::deallocate(self.get_allocator(), ptr->m_data, self.get_chain_capacity());
                allocator_traits_data_t::deallocate(self.get_data_allocator(), ptr, 1);
                ptr = next;
            }
        }

        constexpr void proc_add_chain(this auto&& self)
        {
            auto& comp = self.get_comp();
            chain_data_t* new_chain { };
            if (comp.m_buffer == nullptr)
            {
                new_chain = allocator_traits_data_t::allocate(self.get_data_allocator(), 1);
                allocator_traits_data_t::construct(self.get_data_allocator(), new_chain, comp.m_chain_end, nullptr, allocator_traits_t::allocate(self.get_allocator(), self.get_chain_capacity()));
            }
            else
            {
                chain_data_t* tmp = comp.m_buffer->m_next;
                new_chain = comp.m_buffer;
                comp.m_buffer = tmp;
                new_chain->m_prev = comp.m_chain_end;
                new_chain->m_next = nullptr;
            }
            comp.m_chain_end->m_next = new_chain;
            comp.m_chain_end = new_chain;
            comp.m_data_end = new_chain->m_data;
        }

        constexpr void proc_sub_chain(this auto&& self) noexcept
        {
            auto& comp = self.get_comp();
            chain_data_t* sub_chain = comp.m_chain_end;
            comp.m_chain_end = sub_chain->m_prev;
            comp.m_data_end = comp.m_chain_end->m_data + self.get_chain_capacity();
            sub_chain->m_prev = nullptr;
            sub_chain->m_next = comp.m_buffer;
            comp.m_buffer = sub_chain;
        }

    public:
        constexpr chain(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, compressed_pair_tag::one, alloc, nullptr, nullptr, nullptr, nullptr)
        {
            auto& comp = this->get_comp();
            comp.m_data = allocator_traits_data_t::allocate(this->get_data_allocator(), 1);
            allocator_traits_data_t::construct(this->get_data_allocator(), comp.m_data, nullptr, nullptr, nullptr);
            comp.m_chain_end = comp.m_data;
            comp.m_data->m_data = allocator_traits_t::allocate(this->get_allocator(), this->get_chain_capacity());
            comp.m_data_end = comp.m_data->m_data;
        }

        ~chain()
        {
            this->proc_chain_dealloc(this->get_comp().m_data);
            this->proc_chain_dealloc(this->get_comp().m_buffer);
        }

        template <typename... Tys>
        constexpr void emplace_back(Tys&&... args)
        {
            auto& comp = this->get_comp();
            if (this->is_chain_data_full())
            { this->proc_add_chain(); }
            allocator_traits_t::construct(this->get_allocator(), comp.m_data_end, std::forward<Tys>(args)...);
            ++comp.m_data_end;
        }
        constexpr void push_back(const T& arg)  { this->emplace_back(arg); }
        constexpr void push_back(T&& arg) const { this->emplace_back(std::move(arg)); }
        constexpr void pop_back()
        {
            auto& comp = this->get_comp();
            if (this->is_chain_data_empty())
            { this->proc_sub_chain(); }
            allocator_traits_t::destroy(this->get_allocator(), comp.m_data_end);
            --comp.m_data_end;
            if (this->is_chain_data_empty())
            { this->proc_sub_chain(); }
        }

        [[nodiscard]]
        constexpr T& back() noexcept
        {
            auto& comp = this->get_comp();
            return *(comp.m_data_end - 1);
        }
    };
} // namespace sia
