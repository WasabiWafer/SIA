#pragma once

#include <memory>

#include "SIA/internals/types.hpp"
#include "SIA/utility/compressed_pair.hpp"

namespace sia
{
    template <typename T, size_t N, typename Alloc = std::allocator<T>>
    struct lane
    {
    private:
        using allocator_traits_t = std::allocator_traits<Alloc>;

        compressed_pair<Alloc, T*> m_compair;
        T* m_end_point;

    public:
        constexpr lane(const Alloc& alloc = Alloc()) : m_compair(compressed_pair_tag::one, alloc, nullptr), m_end_point(nullptr)
        {
            m_compair.second() = allocator_traits_t::allocate(m_compair.first(), N);
            m_end_point = m_compair.second();
        }

        ~lane() { allocator_traits_t::deallocate(m_compair.first(), m_compair.second(), N); }

        constexpr T* begin() noexcept   { return m_compair.second(); }
        constexpr T* end() noexcept     { return m_end_point; }
        constexpr size_t size() noexcept { return end() - begin(); }

        template <typename Ty>
        constexpr bool try_push_back(Ty&& arg) noexcept
        {
            if (size() >= N) { return false; }
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
            if (size() >= N) { return false; }
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
