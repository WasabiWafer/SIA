#pragma once

#include <type_traits>
#include <memory>
#include <bit>
#include <cmath>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace align_wrapper_detail
    {
        template <typename T, size_t Align>
        consteval size_t get_require_space() noexcept
        {
            if (sizeof(T) <= Align)
            { return Align; }
            else
            { return Align * std::ceil(float(sizeof(T)) / float(Align)); }
        }

        template <typename T, size_t Align>
            requires (1 <= Align)
        struct align_wrapper_impl
        {
            chunk<byte_t, get_require_space<T, Align>()> m_data;

            constexpr T* get_ptr() noexcept { return type_cast<T*>(static_cast<byte_t*>(this->m_data.ptr())); }
            constexpr const T* get_ptr() const noexcept { return type_cast<const T*>(static_cast<const byte_t*>(this->m_data.ptr())); }
            constexpr T& get_ref() noexcept { return *(this->get_ptr()); }
            constexpr const T& get_ref() const noexcept { return *(this->get_ptr()); }
        };
    } // namespace align_wrapper_detail
    
    template <typename T, size_t Align = sizeof(T)>
    struct align_wrapper : private align_wrapper_detail::align_wrapper_impl<T, Align>
    {
    private:
        using base_t = align_wrapper_detail::align_wrapper_impl<T, Align>;

    public:
        constexpr align_wrapper() noexcept(noexcept(std::construct_at(this->ptr())))
            : base_t()
        { std::construct_at(this->ptr()); }

        constexpr align_wrapper(const align_wrapper& arg) noexcept(noexcept(std::construct_at(this->ptr(), arg.ref())))
            : base_t()
        { std::construct_at(this->ptr(), arg.ref()); }

        constexpr align_wrapper(align_wrapper&& arg) noexcept(noexcept(std::construct_at(this->ptr(), std::move(arg.ref()))))
            : base_t()
        { std::construct_at(this->ptr(), std::move(arg.ref())); }

        template <typename Ty, typename... Tys>
            requires (!std::is_same_v<std::remove_cvref_t<Ty>, align_wrapper>)
        constexpr align_wrapper(Ty&& arg, Tys&&... args) noexcept(noexcept(std::construct_at(this->ptr(), std::forward<Ty>(arg), std::forward<Tys>(args)...)))
            : base_t()
        { std::construct_at(this->ptr(), std::forward<Ty>(arg), std::forward<Tys>(args)...); }

        ~align_wrapper() noexcept(noexcept(std::destroy_at(this->ptr())))
        { std::destroy_at(this->ptr()); }

        constexpr align_wrapper& operator=(const align_wrapper& arg) noexcept(noexcept(this->ref() = arg.ref()))
        {
            if (this->ptr() != arg.ptr())
            { this->ref() = arg.ref(); }
            return *this;
        }

        constexpr align_wrapper& operator=(const T& arg) noexcept(noexcept(this->ref() = arg))
        {
            if (this->ptr() != &arg)
            { this->ref() = arg; }
            return *this;
        }

        constexpr align_wrapper& operator=(T& arg) noexcept(noexcept(this->ref() = std::move(arg)))
        {
            if (this->ptr() != &arg)
            { this->ref() = std::move(arg); }
            return *this;
        }

        constexpr T& ref() noexcept
        { return this->get_ref(); }

        constexpr const T& ref() const noexcept
        { return this->get_ref(); }

        constexpr T* ptr() noexcept
        { return this->get_ptr(); }

        constexpr const T* ptr() const noexcept
        { return this->get_ptr(); }
        
        constexpr T* operator->() noexcept
        { return this->get_ptr(); }

        constexpr const T* operator->() const noexcept
        { return this->get_ptr(); }
    };
} // namespace sia
