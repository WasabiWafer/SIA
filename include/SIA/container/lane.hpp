#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    template <typename T, size_t Size, typename Allocator = std::allocator<T>>
    struct lane
    {
    private:
        using allocator_traits_t = std::allocator_traits<Allocator>;

        compressed_pair<Allocator, T*> m_compair;
        T* m_end_point;

    public:
        constexpr lane(const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr), m_end_point(nullptr)
        {
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), Size);
            m_end_point = m_compair.second();
        }

        constexpr lane(std::initializer_list<T> arg, const Allocator& alloc = Allocator())
            : m_compair(compressed_pair_tag::one, alloc, nullptr), m_end_point(nullptr)
        {
            static_assert(arg.size() <= Size);
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), Size);
            m_end_point = m_compair.second();
            for(auto& elem : arg)
            {
                allocator_traits_t::construct(m_compair.first(), m_end_point++, elem);
            }
        }

        ~lane()
        {
            allocator_traits_t::deallocate(m_compair.first(), m_compair.second(), Size);
        }

        constexpr T* begin()                       noexcept { return m_compair.second(); }
        constexpr T* end()                         noexcept { return m_end_point; }
        constexpr size_t size()                    noexcept { return end() - begin(); }
        constexpr size_t capacity()                noexcept { return Size; }
        constexpr bool is_full()                   noexcept { return size() >= capacity(); }
        constexpr T& operator[](const size_t& idx) noexcept { return *(m_compair.second() + idx); }

        template <typename Ty>
        constexpr bool try_push_back(Ty&& arg) noexcept
        {
            if (size() >= Size) { return false; }
            else
            {
                *m_end_point = std::forward<Ty>(arg);
                ++m_end_point;
                return true;
            }
        }
        
        template <typename... Tys>
        constexpr bool try_emplace_back(Tys&&... args) noexcept
        {
            if (size() >= Size) { return false; }
            else
            {
                allocator_traits_t::construct(m_compair.first(), m_end_point, std::forward<Tys>(args)...);
                ++m_end_point;
                return true;
            }
        }

        template <typename Ty>
        constexpr bool try_pop_back(Ty&& ret) noexcept
        {
            if (size() == 0) { return false; }
            else
            {
                --m_end_point;
                ret = std::move(*m_end_point);
                return true;
            }
        }
    };
} // namespace sia
