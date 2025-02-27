#pragma once

#include <new>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace align_wrapper_detail
    {
        template <typename T, size_t Align> requires (alignof(T) <= Align)
        struct align_wrapper_impl
        {
            chunk<unsigned_interger_t<1>, Align> m_data;

            constexpr T* get_ptr() noexcept { return reinterpret_cast<T*>(this->m_data.m_bin); }
            constexpr const T* get_ptr() const noexcept { return reinterpret_cast<const T*>(this->m_data.m_bin); }
            constexpr T& get_ref() noexcept { return *(this->get_ptr()); }
            constexpr const T& get_ref() const noexcept { return *(this->get_ptr()); }
        };
    } // namespace align_wrapper_detail
    
    template <typename T, size_t Align>
    struct alignas(Align) align_wrapper
    {
    private:
        using impl_t = align_wrapper_detail::align_wrapper_impl<T, Align>;
        impl_t m_impl;

    public:
        constexpr align_wrapper() : m_impl() { }

        constexpr align_wrapper(const align_wrapper& arg) : m_impl()
        { new(this->m_impl.get_ptr()) T(arg.m_impl.get_ref()); }

        constexpr align_wrapper(align_wrapper&& arg) : m_impl()
        { new(this->m_impl.get_ptr()) T(std::move(arg.m_impl.get_ref())); }

        template <typename Ty, typename... Tys>
            requires (!std::is_same_v<std::remove_cvref_t<Ty>, align_wrapper>)
        constexpr align_wrapper(Ty&& arg, Tys&&... args) : m_impl()
        { new(m_impl.get_ptr()) T(std::forward<Ty>(arg), std::forward<Tys>(args)...); }

        ~align_wrapper() { this->m_impl.get_ref()->~T(); }

        constexpr auto& ref(this auto&& self) noexcept
        { return self.m_impl.get_ref(); }

        constexpr auto ptr(this auto&& self) noexcept
        { return self.m_impl.get_ptr(); }
        
        constexpr T* operator->(this auto&& self) noexcept
        { return self.m_impl.get_ptr(); }
    };

    template <typename T> using false_share = align_wrapper<T, std::hardware_destructive_interference_size>;
    template <typename T> using true_share  = align_wrapper<T, std::hardware_constructive_interference_size>;
} // namespace sia
