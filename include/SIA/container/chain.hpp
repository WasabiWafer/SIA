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
            chain_data<T>* m_chain_front;
            T* m_chain_front_data_begin;
            chain_data<T>* m_chain_back;
            T* m_chain_back_data_end;
            chain_data<T>* m_chain_buffer;
        };
    } // namespace chain_detail
    
    template <typename T, size_t Size, typename DataAllocator = std::allocator<T>, typename ChainAllocator = std::allocator<T>>
        requires (Size >= 1)
    struct chain
    {
    private:
        using composition_t = chain_detail::chain_composition<T>;
        using chain_data_t = chain_detail::chain_data<T>;

        using chain_allocator_t = std::allocator_traits<ChainAllocator>::template rebind_alloc<chain_data_t>;
        using data_allocator_t = DataAllocator;

        using chain_allocator_traits_t = std::allocator_traits<chain_allocator_t>;
        using data_allocator_traits_t = std::allocator_traits<data_allocator_t>;

        using composition_compair_t = compressed_pair<chain_allocator_t, composition_t>;
        
        compressed_pair<data_allocator_t, composition_compair_t> m_compair;

        constexpr composition_t& get_composition(this auto&& self) noexcept { return self.m_compair.second().second(); }
        constexpr data_allocator_t& get_data_allocator(this auto&& self) noexcept { return self.m_compair.first(); }
        constexpr chain_allocator_t& get_chain_allocator(this auto&& self) noexcept { return self.m_compair.second().first(); }
        constexpr size_t data_capacity(this auto&& self) noexcept { return Size; }

        constexpr void proc_dealloc(this auto&& self, chain_data_t* ptr) noexcept
        {
            chain_allocator_t& c_alloc = self.get_chain_allocator();
            data_allocator_t& d_alloc = self.get_data_allocator();
            while (ptr != nullptr)
            {
                chain_data_t* next = ptr->m_next;
                data_allocator_traits_t::deallocate(d_alloc, ptr->m_data, self.data_capacity());
                chain_allocator_traits_t::deallocate(c_alloc, ptr, 1);
                ptr = next;
            }
        }

        constexpr size_t is_chain_back_empty(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            if (comp.m_chain_front == comp.m_chain_back)
            { return comp.m_chain_front_data_begin == comp.m_chain_back_data_end; }
            else
            { return comp.m_chain_back_data_end == comp.m_chain_back->m_data; }
        }

        constexpr bool is_chain_front_remain(this auto&& self) noexcept
        {
            composition_t& comp = self->get_composition();
            if (comp.m_chain_front == comp.m_chain_back)
            { return comp.m_chain_front_data_begin != comp.m_chain_back_data_end; }
            else
            { return comp.m_chain_front_data_begin != (comp.m_chain_front->m_data + self.data_capacity()); }
        }

        constexpr bool is_add_front_stuck(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_chain_front_data_begin == comp.m_chain_front->m_data;
        }

        constexpr bool is_full_chain_back(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            return comp.m_chain_back_data_end == (comp.m_chain_back->m_data + self.data_capacity());
        }

        constexpr chain_data_t* gen_new_chain_data(this auto&& self)
        {
            chain_data_t* ret = chain_allocator_traits_t::allocate(self.get_chain_allocator(), 1);
            chain_allocator_traits_t::construct(self.get_chain_allocator(), ret, nullptr, nullptr, nullptr);
            ret->m_data = data_allocator_traits_t::allocate(self.get_data_allocator(), self.data_capacity());
            return ret;
        }

        constexpr chain_data_t* get_able_buffer(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            chain_data_t* ret = comp.m_chain_buffer;
            if (ret != nullptr)
            {
                comp.m_chain_buffer = ret->m_next;
                ret->m_next = nullptr;
                ret->m_prev = nullptr;
            }
            return ret;
        }

        constexpr chain_data_t* get_chain_back_pop(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            chain_data_t* ret = comp.m_chain_back;
            if (ret->m_prev != nullptr)
            {
                comp.m_chain_back = ret->m_prev;
                comp.m_chain_back->m_next = nullptr;
                comp.m_chain_back_data_end = comp.m_chain_back->m_data + self.data_capacity();
                return ret;
            }
            else
            { return nullptr; }
        }

        constexpr chain_data_t* get_chain_front_pop(this auto&& self) noexcept
        {
            composition_t& comp = self.get_composition();
            chain_data_t* ret = comp.m_chain_front;
            if (ret->m_next != nullptr)
            {
                comp.m_chain_front = ret->m_next;
                comp.m_chain_front->m_prev = nullptr;
                comp.m_chain_front_data_begin = comp.m_chain_front->m_data;
                return ret;
            }
            else
            { return nullptr; }
        }


        constexpr void buffer_chain_push_front(this auto&& self, chain_data_t* ptr) noexcept
        {
            composition_t& comp = self.get_composition();
            if (ptr != nullptr)
            {
                ptr->m_next = comp.m_chain_buffer;
                comp.m_chain_buffer = ptr;
            }
        }

        constexpr void chain_push_back(this auto&& self, chain_data_t* ptr)
        {
            composition_t& comp = self.get_composition();
            if (ptr == nullptr)
            { ptr = self.gen_new_chain_data(); }
            ptr->m_prev = comp.m_chain_back;
            comp.m_chain_back->m_next = ptr;
            comp.m_chain_back = ptr;
            comp.m_chain_back_data_end = ptr->m_data;
        }

        constexpr void chain_push_front(this auto&& self, chain_data_t* ptr)
        {
            composition_t& comp = self.get_composition();
            if (ptr == nullptr)
            { ptr = self.gen_new_chain_data(); }
            ptr->m_next = comp.m_chain_front;
            comp.m_chain_front->m_prev = ptr;
            comp.m_chain_front = ptr;
            comp.m_chain_front_data_begin = comp.m_chain_front->m_data + self.data_capacity();
        }


    public:
        constexpr chain(const DataAllocator& data_alloc = DataAllocator(), const ChainAllocator& chain_alloc = ChainAllocator())
            : m_compair(splits::one_v, data_alloc, splits::one_v, chain_alloc, nullptr, nullptr, nullptr, nullptr, nullptr)
        {
            composition_t& comp = this->get_composition();
            chain_allocator_t& c_alloc = this->get_chain_allocator();
            data_allocator_t& d_alloc = this->get_data_allocator();

            comp.m_chain_front = chain_allocator_traits_t::allocate(c_alloc, 1);
            chain_allocator_traits_t::construct(c_alloc, comp.m_chain_front, nullptr, nullptr, nullptr);
            comp.m_chain_front->m_data = data_allocator_traits_t::allocate(d_alloc, this->data_capacity());

            comp.m_chain_back = comp.m_chain_front;
            comp.m_chain_back_data_end = comp.m_chain_back->m_data;
            comp.m_chain_front_data_begin = comp.m_chain_back->m_data;
        }

        ~chain()
        {
            composition_t& comp = this->get_composition();
            this->proc_dealloc(comp.m_chain_front);
            this->proc_dealloc(comp.m_chain_buffer);
        }

        template <typename... Tys>
        constexpr void emplace_front(Tys&&... args) noexcept(noexcept(T(Tys(args)...)))
        {
            composition_t& comp = this->get_composition();
            if (this->is_add_front_stuck())
            { this->chain_push_front(this->get_able_buffer()); }
            --comp.m_chain_front_data_begin;
            data_allocator_traits_t::construct(this->get_data_allocator(), comp.m_chain_front_data_begin, std::forward<Tys>(args)...);
        }
        constexpr void push_front(const T& arg) noexcept(noexcept(this->emplace_front(arg))) { this->emplace_front(arg); }
        constexpr void push_front(T&& arg) noexcept(noexcept(this->emplace_front(std::move(arg)))) { this->emplace_front(std::move(arg)); }
        constexpr void pop_front() noexcept(noexcept(T().~T()))
        {
            composition_t& comp = this->get_composition();
            data_allocator_t& d_alloc = this->get_data_allocator();

            if (comp.m_chain_front == comp.m_chain_back)
            {
                if (comp.m_chain_front_data_begin != comp.m_chain_back_data_end)
                {
                    data_allocator_traits_t::destroy(d_alloc, comp.m_chain_front_data_begin);
                    ++comp.m_chain_front_data_begin;
                }
                if (comp.m_chain_front_data_begin == comp.m_chain_back_data_end)
                {
                    comp.m_chain_front_data_begin = comp.m_chain_front->m_data;
                    comp.m_chain_back_data_end = comp.m_chain_front_data_begin;
                }
            }
            else
            {
                if (comp.m_chain_front_data_begin != comp.m_chain_back_data_end)
                {
                    data_allocator_traits_t::destroy(d_alloc, comp.m_chain_front_data_begin);
                    ++comp.m_chain_front_data_begin;
                }
                if (!this->is_chain_front_remain())
                { this->buffer_chain_push_front(this->get_chain_front_pop()); }
            }            
        }

        [[nodiscard]]
        constexpr T& front() noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_chain_front_data_begin);
        }
        [[nodiscard]]
        constexpr const T& front() const noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_chain_front_data_begin);
        }

        template <typename... Tys>
        constexpr void emplace_back(Tys&&... args) noexcept(noexcept(T(Tys(args)...)))
        {
            composition_t& comp = this->get_composition();
            if (this->is_full_chain_back())
            { this->chain_push_back(this->get_able_buffer()); }
            data_allocator_traits_t::construct(this->get_data_allocator(), comp.m_chain_back_data_end, std::forward<Tys>(args)...);
            ++comp.m_chain_back_data_end;
        }
        constexpr void push_back(const T& arg) noexcept(noexcept(this->emplace_back(arg))) { this->emplace_back(arg); }
        constexpr void push_back(T&& arg) noexcept(noexcept(this->emplace_back(std::move(arg)))) { this->emplace_back(std::move(arg)); }
        constexpr void pop_back() noexcept(noexcept(T().~T()))
        {
            composition_t& comp = this->get_composition();
            data_allocator_t& d_alloc = this->get_data_allocator();
            if (comp.m_chain_front == comp.m_chain_back)
            {
                if (comp.m_chain_front_data_begin != comp.m_chain_back_data_end)
                {
                    --comp.m_chain_back_data_end;
                    data_allocator_traits_t::destroy(d_alloc, comp.m_chain_back_data_end);
                }
                if (comp.m_chain_front_data_begin == comp.m_chain_back_data_end)
                {
                    comp.m_chain_front_data_begin = comp.m_chain_front->m_data;
                    comp.m_chain_back_data_end = comp.m_chain_front_data_begin;
                }
            }
            else
            {
                if (comp.m_chain_front_data_begin != comp.m_chain_back_data_end)
                {
                    --comp.m_chain_back_data_end;
                    data_allocator_traits_t::destroy(this->get_data_allocator(), comp.m_chain_back_data_end);
                }
                if (this->is_chain_back_empty())
                { this->buffer_chain_push_front(this->get_chain_back_pop()); }
            }
        }

        [[nodiscard]]
        constexpr T& back() noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_chain_back_data_end - 1);
        }

        [[nodiscard]]
        constexpr const T& back() const noexcept
        {
            composition_t& comp = this->get_composition();
            return *(comp.m_chain_back_data_end - 1);
        }
    };
} // namespace sia
