#pragma once

#include <memory>
#include <scoped_allocator>
#include <atomic>

#include "SIA/container/ring.hpp"
#include "SIA/concurrency/internals/types.hpp"
#include "SIA/concurrency/utility/state.hpp"
#include "SIA/concurrency/utility/tools.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace ring_detail
        {
            enum class ring_action_state { poped = 0, pushed };

            template <typename Derived>
            struct state_composition_base
            {
                private:
                    using action_state_type = state<ring_action_state>;
                    using position_state_type = std::atomic<size_t>;
                    using position_state_value_type = size_t;
                    friend Derived;
                    constexpr state_composition_base() noexcept = default;
                    constexpr ~state_composition_base() noexcept = default;
                public:
                    template <typename T = void>
                        requires (requires (Derived derv) { derv.m_last_action_state; })
                    constexpr auto& get_last_action() noexcept { return static_cast<Derived*>(this)->m_last_action_state.ref(); }

                    template <typename T = void>
                        requires (requires (Derived derv) { derv.m_output_state; })
                    constexpr auto& get_output_state() noexcept { return static_cast<Derived*>(this)->m_output_state.ref(); }

                    template <typename T = void>
                        requires (requires (Derived derv) { derv.m_input_state; })
                    constexpr auto& get_input_state() noexcept { return static_cast<Derived*>(this)->m_input_state.ref(); }

                    static constexpr bool position_enter(position_state_type& state, auto ring_counter) noexcept
                    {
                        size_t target_count = ring_counter.count();
                        if (target_count == state.load(std::memory_order::relaxed))
                        { return state.compare_exchange_strong(target_count, ring_counter.next_cycle(), std::memory_order_relaxed, std::memory_order_relaxed); }
                        else
                        { return false; }
                    }

                    static constexpr void action_wait(action_state_type& state, ring_action_state expt) noexcept
                    { while(!(expt == state.status())) { } }

                    static constexpr void action_set(action_state_type& state, ring_action_state value) noexcept
                    { state.set(value); }
            };

            template <tags::producer PTag, tags::consumer CTag>
            struct state_composition;

            template <>
            struct state_composition<tags::producer::single, tags::consumer::single> : public state_composition_base<state_composition<tags::producer::single, tags::consumer::single>>
            {
                private:
                    using base_type = state_composition_base<state_composition<tags::producer::single, tags::consumer::single>>;
                public:
            };

            template <>
            struct state_composition<tags::producer::single, tags::consumer::multiple> : public state_composition_base<state_composition<tags::producer::single, tags::consumer::multiple>>
            {
                private:
                    using base_type = state_composition_base<state_composition<tags::producer::single, tags::consumer::multiple>>;
                public:
                    true_share<base_type::action_state_type> m_last_action_state;
                    true_share<base_type::position_state_type> m_output_state;

                    constexpr state_composition(base_type::action_state_type action, base_type::position_state_value_type pos) noexcept
                        : m_last_action_state(action), m_output_state(pos)
                    { }
            };
            
            template <>
            struct state_composition<tags::producer::multiple, tags::consumer::single> : public state_composition_base<state_composition<tags::producer::multiple, tags::consumer::single>>
            {
                private:
                    using base_type = state_composition_base<state_composition<tags::producer::multiple, tags::consumer::single>>;
                public:
                    true_share<base_type::action_state_type> m_last_action_state;
                    true_share<base_type::position_state_type> m_input_state;

                    constexpr state_composition(base_type::action_state_type action, base_type::position_state_value_type pos) noexcept
                        : m_last_action_state(action), m_input_state(pos)
                    { }
            };


            template <>
            struct state_composition<tags::producer::multiple, tags::consumer::multiple> : public state_composition_base<state_composition<tags::producer::multiple, tags::consumer::multiple>>
            {
                private:
                    using base_type = state_composition_base<state_composition<tags::producer::multiple, tags::consumer::multiple>>;
                public:
                    true_share<base_type::action_state_type> m_last_action_state;
                    true_share<base_type::position_state_type> m_input_state;
                    true_share<base_type::position_state_type> m_output_state;

                    constexpr state_composition(base_type::action_state_type action, base_type::position_state_value_type pos) noexcept
                        : m_last_action_state(action), m_input_state(pos), m_output_state(pos)
                    { }
            };

            template <typename Derived>
            struct ring_composition_base;

            template <template <typename, size_t, tags::producer, tags::consumer> typename Derived, typename T, size_t Size, tags::producer PTag, tags::consumer CTag>
            struct ring_composition_base<Derived<T, Size, PTag, CTag>>
            {
                    private:
                        using ring_counter_type = sia::ring_detail::ring_counter<size_t, Size>;
                        using atomic_type = std::atomic<ring_counter_type>;
                        using state_composition_type = state_composition<PTag, CTag>;
                        using derived_type = Derived<T, Size, PTag, CTag>;
                        friend derived_type;
                        constexpr ring_composition_base() noexcept = default;
                        constexpr ~ring_composition_base() noexcept = default;
                    public:
                        template <typename T = void>
                            requires (requires (derived_type derv) { derv.m_begin; })
                        constexpr auto& get_begin_atomic() noexcept
                        { return static_cast<derived_type*>(this)->m_begin.ref(); }

                        template <typename T = void>
                            requires (requires (derived_type derv) { derv.m_end; })
                        constexpr auto& get_end_atomic() noexcept
                        { return static_cast<derived_type*>(this)->m_end.ref(); }

                        template <typename T = void>
                            requires (requires (derived_type derv) { derv.m_data_entry; })
                        constexpr auto& get_data() noexcept
                        { return static_cast<derived_type*>(this)->m_data_entry.get<0>(); }

                        template <typename T = void>
                            requires (requires (derived_type derv) { derv.m_data_entry; })
                        constexpr auto& get_state_composition_data() noexcept
                        { return static_cast<derived_type*>(this)->m_data_entry.get<1>(); }
            };

            template <typename T, size_t Size, tags::producer PTag, tags::consumer CTag>
            struct ring_composition;

            template <typename T, size_t Size, tags::producer PTag, tags::consumer CTag>
                requires ((PTag != tags::producer::multiple) && (CTag != tags::consumer::multiple))
            struct ring_composition<T, Size, PTag, CTag> : public ring_composition_base<ring_composition<T, Size, PTag, CTag>>
            {
                private:
                    using base_type = ring_composition_base<ring_composition<T, Size, PTag, CTag>>;
                public:
                    true_share<base_type::template atomic_type> m_begin;
                    true_share<base_type::template atomic_type> m_end;
                    false_share<T*> m_data_entry;
            };

            template <typename T, size_t Size, tags::producer PTag, tags::consumer CTag>
                requires ((PTag == tags::producer::multiple) || (CTag == tags::consumer::multiple))
            struct ring_composition<T, Size, PTag, CTag> : public ring_composition_base<ring_composition<T, Size, PTag, CTag>>
            {
                private:
                    using base_type = ring_composition_base<ring_composition<T, Size, PTag, CTag>>;
                public:
                    true_share<base_type::template atomic_type> m_begin;
                    true_share<base_type::template atomic_type> m_end;
                    false_share<T*, base_type::template state_composition_type*> m_data_entry;
            };

            template <typename Derived>
            struct ring_base;

            template <template <typename, size_t, tags::producer, tags::consumer, typename> typename Ring, typename T, size_t Size, tags::producer PTag, tags::consumer CTag, typename Alloc>
            struct ring_base<Ring<T, Size, PTag, CTag, Alloc>>
            {
                private:
                    using derived_type = Ring<T, Size, PTag, CTag, Alloc>;
                    using ring_counter_type = sia::ring_detail::ring_counter<size_t, Size>;
                    constexpr ring_base() noexcept = default;
                    constexpr ~ring_base() noexcept = default;
                    friend derived_type;

                    static constexpr size_t size(size_t beg, size_t end) noexcept
                    {
                        constexpr const size_t adj = ring_counter_type::adjustment();
                        if (end >= beg)
                        { return end - beg; }
                        else
                        { return end - beg - adj; }
                    }
                    static constexpr bool is_empty(size_t beg, size_t end) noexcept { return size(beg, end) == 0; }
                    static constexpr bool is_full(size_t beg, size_t end) noexcept { return size(beg, end) == Size; }
                    static constexpr bool is_multiple_producer() noexcept { return PTag == tags::producer::multiple; }
                    static constexpr bool is_multiple_consumer() noexcept { return CTag == tags::consumer::multiple; }
                    static constexpr bool is_multiple() noexcept { return is_multiple_producer() || is_multiple_consumer(); }

                public:
                    static constexpr size_t capacity() noexcept { return Size; }

                    template <tags::loop LoopTag, tags::wait WaitTag, typename... Tys>
                    constexpr bool loop_emplace_back(auto ltt_v, auto wtt_v, Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &derived_type::template try_emplace_back<Tys...>, static_cast<derived_type*>(this), std::forward<Tys>(args)...); }
                    template <typename... Tys>
                    constexpr void emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Tys>(args)...); }
                    constexpr bool try_push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                    { return static_cast<derived_type*>(this)->try_emplace_back(arg); }
                    constexpr bool try_push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                    { return static_cast<derived_type*>(this)->try_emplace_back(std::move(arg)); }
                    constexpr void push_back(const T& arg) noexcept(std::is_nothrow_constructible_v<T, const T&>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
                    constexpr void push_back(T&& arg) noexcept(std::is_nothrow_constructible_v<T, T&&>)
                    { loop_emplace_back<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::move(arg)); }

                    template <tags::loop LoopTag, tags::wait WaitTag, typename Ty>
                    constexpr bool loop_extract_front(auto ltt_v, auto wtt_v, Ty&& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                        )
                    { return loop<LoopTag, WaitTag>(true, ltt_v, wtt_v, &derived_type::template try_extract_front<Ty>, static_cast<derived_type*>(this), std::forward<Ty>(arg));}

                    template <typename Ty>
                    constexpr void extract_front(Ty&& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                            (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                        )
                    { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, std::forward<Ty>(arg)); }

                    constexpr bool try_pull_front(T& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                            (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>))
                        )
                    { return static_cast<derived_type*>(this)->try_extract_front(arg); }

                    constexpr void pull_front(T& arg)
                        noexcept
                        (
                            std::is_nothrow_destructible_v<T> &&
                            ((std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                            (!std::is_move_assignable_v<T> && std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>))
                        )
                    { loop_extract_front<tags::loop::busy, tags::wait::busy>(stamps::basis::empty_loop_val, stamps::basis::empty_wait_val, arg); }
            };
        } // namespace ring_detail

        template <typename T, size_t Size, tags::producer PTag = tags::producer::single, tags::consumer CTag = tags::consumer::single, typename Alloc = std::scoped_allocator_adaptor<std::allocator<T>, std::allocator<ring_detail::state_composition<PTag, CTag>>>>
            requires (Size <= (std::numeric_limits<size_t>::max()/2))
        struct ring : public ring_detail::ring_base<ring<T, Size, PTag, CTag, Alloc>>
        {
            private:
                using base_type = ring_detail::ring_base<ring<T, Size, PTag, CTag, Alloc>>;
                using action_state_value_type = ring_detail::ring_action_state;
                using allocator_type = Alloc;
                using composition_type = ring_detail::ring_composition<T, Size, PTag, CTag>;
                
                compressed_pair<allocator_type, composition_type> m_compair;

                static constexpr void end_atomic_counter_inc(auto& target_atomic, auto expect, auto desire, auto beg_counter) noexcept
                {
                    auto less_op =
                        [count = beg_counter.count()] (auto lc, auto rc) noexcept
                        { return base_type::size(count, lc.count()) < base_type::size(count, rc.count()); };
                    while_expression_exchange_weak(less_op, target_atomic, expect, desire, std::memory_order::relaxed, std::memory_order::relaxed);
                }

                static constexpr void beg_atomic_counter_inc(auto& target_atomic, auto expect, auto desire, auto end_counter) noexcept
                {
                    auto greater_op =
                        [count = end_counter.count()] (auto lc, auto rc) noexcept
                        { return base_type::size(lc.count(), count) > base_type::size(rc.count(), count); };
                    while_expression_exchange_weak(greater_op, target_atomic, expect, desire, std::memory_order::relaxed, std::memory_order::relaxed);
                }

            public:
                using outer_allocator_value_type = T;
                using inner_allocator_value_type = ring_detail::state_composition<PTag, CTag>;
                using outer_allocator_type = allocator_type::outer_allocator_type;
                using inner_allocator_type = allocator_type::inner_allocator_type::outer_allocator_type;

                composition_type& get_composition() noexcept { return m_compair.second(); }
                outer_allocator_type& get_outer_allocator() noexcept { return m_compair.first().outer_allocator(); }
                inner_allocator_type& get_inner_allocator() noexcept { return m_compair.first().inner_allocator().outer_allocator(); }

                constexpr ring(const allocator_type& alloc = allocator_type{ }) noexcept
                    : m_compair(splits::one_v, alloc)
                {
                    composition_type& comp = get_composition();
                    comp.get_data() = std::allocator_traits<outer_allocator_type>::allocate(get_outer_allocator(), base_type::capacity());

                    for (outer_allocator_value_type* at {comp.get_data()}, *end {at + base_type::capacity()}; at != end; ++at)
                    { std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), at); }

                    if constexpr (base_type::is_multiple())
                    {
                        comp.get_state_composition_data() = std::allocator_traits<inner_allocator_type>::allocate(get_inner_allocator(), base_type::capacity());
                        for
                            (
                                std::tuple<size_t, inner_allocator_value_type*, inner_allocator_value_type*> tpl {0, comp.get_state_composition_data(), comp.get_state_composition_data() + base_type::capacity()};
                                std::get<1>(tpl) != std::get<2>(tpl);
                                ++std::get<0>(tpl), ++std::get<1>(tpl)
                            )
                        { std::allocator_traits<inner_allocator_type>::construct(get_inner_allocator(), std::get<1>(tpl), action_state_value_type::poped, std::get<0>(tpl)); }
                    }
                }

                template <typename... Tys>
                constexpr bool try_emplace_back(Tys&&... args) noexcept(std::is_nothrow_constructible_v<T, Tys...>)
                {
                    composition_type& comp = get_composition();
                    auto& beg = comp.get_begin_atomic();
                    auto& end = comp.get_end_atomic();
                    auto beg_counter = beg.load(std::memory_order::relaxed);
                    auto end_counter = end.load(std::memory_order::relaxed);
                    constexpr auto dck_is_full =
                        [] (auto& beg_counter, auto end_counter, auto& beg_source)
                        {
                            if (base_type::is_full(beg_counter.count(), end_counter.count()))
                            {
                                beg_counter = beg_source.load(std::memory_order::relaxed);
                                if (base_type::is_full(beg_counter.count(), end_counter.count()))
                                { return true; }
                            }
                            return false;
                        };

                    if constexpr (base_type::is_multiple_producer())
                    {
                        while(!dck_is_full(beg_counter, end_counter, beg))
                        {
                            auto state_comp_ptr = comp.get_state_composition_data() + end_counter.offset();
                            if (state_comp_ptr->position_enter(state_comp_ptr->get_input_state(), end_counter))
                            {
                                state_comp_ptr->action_wait(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::poped);
                                T* target = comp.get_data() + end_counter.offset();
                                std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), target, std::forward<Tys>(args)...);
                                end_counter.inc();
                                end_atomic_counter_inc(end, end.load(std::memory_order::relaxed), end_counter, beg_counter);
                                state_comp_ptr->action_set(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::pushed);
                                return true;
                            }
                            else
                            { end_counter.inc(); }
                        }
                        return false;
                    }
                    else
                    {
                        if (dck_is_full(beg_counter, end_counter, beg))
                        { return false; }
                        else
                        {
                            std::allocator_traits<outer_allocator_type>::construct(get_outer_allocator(), comp.get_data() + end_counter.offset(), std::forward<Tys>(args)...);
                            if constexpr (base_type::is_multiple_consumer())
                            {
                                auto state_comp_ptr = comp.get_state_composition_data() + end_counter.offset();
                                end_counter.inc();
                                end.store(end_counter, std::memory_order::relaxed);
                                state_comp_ptr->action_set(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::pushed);
                            }
                            else
                            {
                                end_counter.inc();
                                end.store(end_counter, std::memory_order::relaxed);
                            }
                            return true;
                        }
                    }
                }

                template <typename Ty>
                    requires (std::is_assignable_v<Ty, T&> || std::is_assignable_v<Ty, T&&>)
                constexpr bool try_extract_front(Ty&& arg)
                    noexcept
                    (
                        std::is_nothrow_destructible_v<T> &&
                        ((std::is_assignable_v<Ty, T&&> && std::is_nothrow_assignable_v<Ty, T&&>) ||
                        (!std::is_assignable_v<Ty, T&&> && std::is_assignable_v<Ty, T&> && std::is_nothrow_assignable_v<Ty, T&>))
                    )
                {
                    composition_type& comp = get_composition();
                    auto& beg = comp.get_begin_atomic();
                    auto& end = comp.get_end_atomic();
                    auto beg_counter = beg.load(std::memory_order::relaxed);
                    auto end_counter = end.load(std::memory_order::relaxed);
                    constexpr auto dck_is_empty =
                        [] (auto beg_counter, auto& end_counter, auto& end_source)
                        {
                            if (base_type::is_empty(beg_counter.count(), end_counter.count()))
                            {
                                end_counter = end_source.load(std::memory_order::relaxed);
                                if (base_type::is_empty(beg_counter.count(), end_counter.count()))
                                { return true; }
                            }
                            return false;
                        };
                    
                    if constexpr (base_type::is_multiple_consumer())
                    {
                        while(!dck_is_empty(beg_counter, end_counter, end))
                        {
                            auto state_comp_ptr = comp.get_state_composition_data() + beg_counter.offset();
                            if (state_comp_ptr->position_enter(state_comp_ptr->get_output_state(), beg_counter))
                            {
                                state_comp_ptr->action_wait(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::pushed);
                                T* target = comp.get_data() + beg_counter.offset();
                                if constexpr (std::is_assignable_v<Ty, T&&>) { arg = std::move(*target); }
                                else { arg = *target; }
                                std::allocator_traits<outer_allocator_type>::destroy(get_outer_allocator(), target);
                                beg_counter.inc();
                                beg_atomic_counter_inc(beg, beg.load(std::memory_order::relaxed), beg_counter, end_counter);
                                state_comp_ptr->action_set(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::poped);
                                return true;
                            }
                            else
                            { beg_counter.inc(); }
                        }
                        return false;
                    }
                    else
                    {
                        if (dck_is_empty(beg_counter, end_counter, end))
                        { return false; }
                        else
                        {
                            T* target = comp.get_data() + beg_counter.offset();
                            if constexpr (std::is_assignable_v<Ty, T&&>) { arg = std::move(*target); }
                            else { arg = *target; }
                            std::allocator_traits<outer_allocator_type>::destroy(get_outer_allocator(), target);
                            if constexpr (base_type::is_multiple_producer())
                            {
                                auto state_comp_ptr = comp.get_state_composition_data() + beg_counter.offset();
                                beg_counter.inc();
                                beg.store(beg_counter, std::memory_order::relaxed);
                                state_comp_ptr->action_set(state_comp_ptr->get_last_action(), ring_detail::ring_action_state::poped);
                            }
                            else
                            {
                                beg_counter.inc();
                                beg.store(beg_counter, std::memory_order::relaxed);
                            }
                            return true;
                        }
                    }
                }
        };
    } // namespace concurrency
} // namespace sia