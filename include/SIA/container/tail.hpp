#pragma once

#include <memory>

#include "SIA/utility/compressed_pair.hpp"
#include "SIA/memory/allocator_converter.hpp"

// should seperate tail_stock functionality to allocator
// this is meaningless

namespace sia
{
    namespace tail_detail
    {
        template <typename T>
        struct tail_node
        {
            tail_node* m_node;
            T* m_data;

            constexpr tail_node* next() noexcept { return m_node; }
            constexpr const tail_node* next() const noexcept { return m_node; }
            constexpr T* ptr() noexcept { return m_data; }
            constexpr const T* ptr() const noexcept { return m_data; }
            constexpr T& ref() noexcept { return *m_data; }
            constexpr const T& ref() const noexcept { return *m_data; }
            constexpr void set_data(T* new_ptr) noexcept { m_data = new_ptr; }
            constexpr void set_node(tail_node* new_ptr) noexcept { m_node = new_ptr; }
            template <typename Allocator>
            constexpr void destroy_data(Allocator&& alloc) noexcept(std::is_nothrow_destructible_v<T>)
            { std::allocator_traits<std::remove_reference_t<Allocator>>::destroy(std::forward<Allocator>(alloc), m_data); }
            template <typename Allocator>
            constexpr void deallocate_data(Allocator&& alloc) noexcept(std::is_nothrow_destructible_v<T>)
            { std::allocator_traits<std::remove_reference_t<Allocator>>::deallocate(std::forward<Allocator>(alloc), m_data, 1); }
        };

        template <typename T>
        struct tail_stock
        {
            private:
                using node_t = tail_node<T>;
                node_t* m_stock;

            public:
                constexpr tail_stock() noexcept = default;
                constexpr tail_stock(node_t* arg) noexcept
                    : m_stock{arg}
                { }

                constexpr bool is_empty() noexcept { return m_stock == nullptr; }
                constexpr node_t* begin() noexcept { return m_stock; }
                constexpr node_t* back() noexcept
                {
                    node_t* ret {begin()};
                    if (ret != nullptr)
                    {
                        while (ret->next() != nullptr)
                        { ret = ret->next(); }
                    }
                    return ret;
                }

                constexpr node_t* pull() noexcept
                {
                    node_t* ret = m_stock;
                    m_stock = ret->next();
                    ret->set_node(nullptr);
                    return ret;
                }

                constexpr void push(node_t* arg) noexcept
                {
                    arg->set_node(m_stock);
                    m_stock = arg;
                }

                constexpr void attach(node_t* arg) noexcept
                {
                    if (is_empty())
                    { m_stock = arg; }
                    else
                    { back()->set_node(arg); }
                }

                constexpr void replace(node_t* arg) noexcept
                { m_stock = arg; }
        };

        template <typename T>
        struct tail_composition
        {
            private:
                using node_t = tail_node<T>;
            public:
                tail_node<T>* m_head;
                tail_stock<T> m_stock;

                constexpr node_t* next() noexcept { return m_head; }
                constexpr const node_t* next() const noexcept { return m_head; }
                constexpr void set_node(node_t* new_ptr) noexcept { m_head = new_ptr; }
        };
    } // namespace tail_detail
    
    template <typename T, typename Allocator = std::allocator<T>>
    struct tail
    {
        private:
            using value_type = T;
            using allocator_type = Allocator;
            using composition_t = tail_detail::tail_composition<T>;
            using node_t = tail_detail::tail_node<T>;
            using allocator_converter_t = allocator_converter<node_t, allocator_type>;
            using converted_allocator_t = allocator_converter_t::converted_allocator_t;

            compressed_pair<allocator_type, composition_t> m_compair;

            constexpr allocator_type& get_allocator() noexcept { return m_compair.first(); }
            constexpr const allocator_type& get_allocator() const noexcept { return m_compair.first(); }
            constexpr composition_t& get_composition() noexcept { return m_compair.second(); }
            constexpr const composition_t& get_composition() const noexcept { return m_compair.second(); }

            template <typename... Tys>
            constexpr value_type* create_data(Tys&&... args)
            {
                value_type* ret = std::allocator_traits<allocator_type>::allocate(get_allocator(), 1);
                std::allocator_traits<allocator_type>::construct(get_allocator(), ret, std::forward<Tys>(args)...);
                return ret;
            }

            constexpr node_t* create_node(node_t* next_node, value_type* data_ptr)
            {
                allocator_converter_t ac {get_allocator()};
                node_t* ret = std::allocator_traits<converted_allocator_t>::allocate(ac.get_allocator(), 1);
                std::allocator_traits<converted_allocator_t>::construct(ac.get_allocator(), ret, next_node, data_ptr);
                return ret;
            }

            constexpr void deallocate_node(node_t* target) noexcept(std::is_nothrow_destructible_v<T>)
            {
                while (target != nullptr)
                {
                    node_t* tmp = target->next();
                    target->deallocate_data(get_allocator());
                    {
                        allocator_converter_t ac {get_allocator()};
                        std::allocator_traits<converted_allocator_t>::deallocate(ac.get_allocator(), target, 1);
                    }
                    target = tmp;
                }
            }

            constexpr void copy_node(const node_t* arg)
            {
                const node_t* target = arg;
                if (target != nullptr)
                {
                    push_front(target->ref());
                    target = target->next();
                    for (node_t* at {get_composition().m_head}; target != nullptr; target = target->next(), at = at->next())
                    { push_after(at, target->ref()); }
                }
            }

        public:
            constexpr tail(const allocator_type& alloc = allocator_type{ }) noexcept 
                : m_compair(splits::one_v, alloc, nullptr, nullptr)
            { }

            constexpr tail(const tail& arg, const allocator_type& alloc = allocator_type{ })
                : m_compair(splits::one_v, alloc, nullptr, nullptr)
            { copy_node(arg.begin()); }

            constexpr tail(tail&& arg, const allocator_type& alloc = allocator_type{ })
                : m_compair(splits::one_v, alloc, arg.get_composition().m_head, arg.get_composition().m_stock)
            {
                arg.get_composition().m_head = nullptr;
                arg.get_composition().m_stock = nullptr;
            }

            constexpr ~tail() noexcept(std::is_nothrow_destructible_v<T>)
            {
                composition_t& comp = get_composition();
                deallocate_node(comp.m_stock.begin());
                deallocate_node(comp.m_head);
            }

            constexpr tail& operator=(const tail& arg)
            {
                clear();
                copy_node(arg.begin());
                return *this;
            }

            constexpr tail& operator=(tail&& arg) noexcept(std::is_nothrow_destructible_v<T>)
            {
                clear();
                composition_t& comp {get_composition()};
                composition_t& target_comp {arg.get_composition()};
                comp.m_head = target_comp.m_head;
                comp.m_stock.attach(target_comp.m_stock.begin());
                target_comp.m_head = nullptr;
                target_comp.m_stock.replace(nullptr);
                return *this;
            }

            constexpr bool is_empty(this auto&& self) noexcept { return self.get_composition().m_head == nullptr; }

            constexpr node_t* begin() noexcept { return get_composition().m_head; }
            constexpr const node_t* begin() const noexcept { return get_composition().m_head; }
            
            template <typename NodeType, typename... Tys>
            constexpr void emplace_after(NodeType* at, Tys&&... args)
            {
                composition_t& comp {get_composition()};
                if (comp.m_stock.is_empty())
                {
                    value_type* new_data = create_data(std::forward<Tys>(args)...);
                    node_t* new_node = create_node(at->next(), new_data);
                    at->set_node(new_node);
                }
                else
                {
                    node_t* new_node = comp.m_stock.pull();
                    new_node->set_node(at->next());
                    std::allocator_traits<allocator_type>::construct(get_allocator(), new_node->ptr(), std::forward<Tys>(args)...);
                    at->set_node(new_node);
                }
            }

            template <typename... Tys>
            constexpr void emplace_front(Tys&&... args)
            { emplace_after(&get_composition(), std::forward<Tys>(args)...); }

            template <typename NodeType>
            constexpr void pop_after(NodeType* at) noexcept(std::is_nothrow_destructible_v<T>)
            {
                node_t* target = at->next();
                at->set_node(target->next());
                target->destroy_data(get_allocator());
                get_composition().m_stock.push(target);
            }

            constexpr void pop_front() noexcept(std::is_nothrow_destructible_v<T>)
            { pop_after(&get_composition()); }

            template <typename NodeType>
            constexpr void push_after(NodeType* at, const T& arg)
            { emplace_after(at, arg); }

            template <typename NodeType>
            constexpr void push_after(NodeType* at, T&& arg)
            { emplace_after(at, std::move(arg)); }

            constexpr void push_front(const T& arg)
            { emplace_after(&get_composition(), arg); }

            constexpr void push_front(T&& arg)
            { emplace_after(&get_composition(), std::move(arg)); }

            constexpr value_type& front() noexcept { return get_composition().m_head->ref(); }
            constexpr const value_type& front() const noexcept { return get_composition().m_head->ref(); }

            constexpr void clear() noexcept(std::is_nothrow_destructible_v<T>)
            {
                composition_t& comp = get_composition();
                node_t* target = begin();
                if (target != nullptr)
                {
                    while(target->next() != nullptr)
                    {
                        target->destroy_data(get_allocator());
                        target = target->next();
                    }
                    target->destroy_data(get_allocator());
                    target->set_node(comp.m_stock.begin());
                    comp.m_stock.replace(comp.m_head);
                    comp.m_head = nullptr;
                }
            }
    };
} // namespace sia
