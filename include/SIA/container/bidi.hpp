#pragma once

#include <memory>

#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    namespace bidi_detail
    {
        template <typename T>
        struct bidi_data
        {
        private:
            bidi_data* m_prev;
            bidi_data* m_next;
            T m_data;
        public:
            constexpr bidi_data() noexcept
                : m_prev(this), m_next(this), m_data()
            { }
            template <typename... Cs>
            constexpr bidi_data(bidi_data* ptr_prev, bidi_data* ptr_next, Cs&&... args) noexcept
                : m_prev(ptr_prev), m_next(ptr_next), m_data(std::forward<Cs>(args)...)
            { }
            constexpr bidi_data*& prev() noexcept { return m_prev; }
            constexpr bidi_data*& next() noexcept { return m_next; }
            constexpr T& data() noexcept { return m_data; }
        };
    } // namespace bidi_detail
    
    template <typename T, typename Allocator = std::allocator<T>>
    struct bidi
    {
    private:
        using bidi_data_t = bidi_detail::bidi_data<T>;
        using alloc_rebind = std::allocator_traits<Allocator>::template rebind_alloc<bidi_data_t>;
        using allocator_traits_t = std::allocator_traits<alloc_rebind>;
        compressed_pair<alloc_rebind, bidi_data_t*> compair;
    public:
        constexpr bidi() noexcept
            : compair(compressed_pair_tag::zero, nullptr, nullptr)
        {
            bidi_data_t* block = allocator_traits_t::allocate(compair.first(), 1);
            allocator_traits_t::construct(compair.first(), block, block, block);
            compair.second() = block;
        }

        constexpr bidi_data_t* begin() noexcept { return compair.second()->next(); }
        constexpr bidi_data_t* end() noexcept { return compair.second(); }
    };
} // namespace sia
