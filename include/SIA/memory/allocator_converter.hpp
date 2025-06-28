#pragma once

#include <memory>

#include "SIA/utility/types/function_pointer.hpp"

namespace sia
{
    template <typename ConvertType, typename Allocator>
    struct allocator_converter
    {
        public:
            using converted_allocator_t = std::allocator_traits<Allocator>::template rebind_alloc<ConvertType>;
        private:
            using allocator_t = Allocator;
            using value_type = ConvertType;
            using allocator_trait_t = std::allocator_traits<converted_allocator_t>;
            
            allocator_t& m_alloc;
            converted_allocator_t m_convert;
            
        public:            
            constexpr allocator_converter(Allocator& alloc)
                noexcept(std::is_nothrow_constructible_v<converted_allocator_t, allocator_t&&>)
                    : m_alloc(alloc), m_convert(std::move(m_alloc))
                { }

            ~allocator_converter() noexcept(std::is_nothrow_assignable_v<allocator_t&, converted_allocator_t&&>)
            { m_alloc = std::move(m_convert); }

            constexpr converted_allocator_t& get_allocator() noexcept
            { return m_convert; }
    };

    template <typename ConvertType, typename Allocator>
    allocator_converter(Allocator&&) -> allocator_converter<ConvertType, std::remove_reference_t<Allocator>>;
} // namespace sia
