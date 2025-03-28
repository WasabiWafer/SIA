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
        constexpr size_t ceil(float arg) noexcept
        {
            size_t integer = static_cast<size_t>(arg);
            return integer < arg ? integer + 1 : integer; 
        }

        template <typename T, size_t Align>
        constexpr size_t get_require_space() noexcept
        {
            if (sizeof(T) <= Align)
            { return Align; }
            else
            { return Align * ceil(float(sizeof(T)) / float(Align)); }
        }
    } // namespace align_wrapper_detail
    
    template <typename T, size_t Align = sizeof(T)>
    struct align_wrapper : public chunk<byte_t, align_wrapper_detail::get_require_space<T, Align>()>
    {
    private:
        using base_t = chunk<byte_t, align_wrapper_detail::get_require_space<T, Align>()>;
        using self_t = align_wrapper;

    public:
        constexpr align_wrapper() noexcept(noexcept(std::construct_at(this->self_t::ptr())))
            : base_t()
        { std::construct_at(this->self_t::ptr()); }

        constexpr align_wrapper(const align_wrapper& arg) noexcept(noexcept(std::construct_at(this->self_t::ptr(), arg.ref())))
            : base_t()
        { std::construct_at(this->self_t::ptr(), arg.ref()); }

        constexpr align_wrapper(align_wrapper&& arg) noexcept(noexcept(std::construct_at(this->self_t::ptr(), std::move(arg.ref()))))
            : base_t()
        { std::construct_at(this->self_t::ptr(), std::move(arg.ref())); }

        template <typename Ty, typename... Tys>
            requires (!std::is_same_v<std::remove_cvref_t<Ty>, align_wrapper>)
        constexpr align_wrapper(Ty&& arg, Tys&&... args) noexcept(noexcept(std::construct_at(this->self_t::ptr(), std::forward<Ty>(arg), std::forward<Tys>(args)...)))
            : base_t()
        { std::construct_at(this->self_t::ptr(), std::forward<Ty>(arg), std::forward<Tys>(args)...); }

        ~align_wrapper() noexcept(noexcept(std::destroy_at(this->self_t::ptr())))
        { std::destroy_at(this->self_t::ptr()); }

        constexpr align_wrapper& operator=(const align_wrapper& arg) noexcept(noexcept(this->ref() = arg.ref()))
        {
            if (this->self_t::ptr() != arg.ptr())
            { this->ref() = arg.ref(); }
            return *this;
        }

        constexpr align_wrapper& operator=(align_wrapper&& arg) noexcept(noexcept(this->ref() = std::move(arg.ref())))
        {
            if (this->self_t::ptr() != arg.ptr())
            { this->ref() = std::move(arg.ref()); }
            return *this;
        }

        constexpr align_wrapper& operator=(const T& arg) noexcept(noexcept(this->ref() = arg))
        {
            if (this->self_t::ptr() != &arg)
            { this->ref() = arg; }
            return *this;
        }

        constexpr align_wrapper& operator=(T&& arg) noexcept(noexcept(this->ref() = std::move(arg)))
        {
            if (this->self_t::ptr() != &arg)
            { this->ref() = std::move(arg); }
            return *this;
        }

        constexpr       T* ptr()        noexcept { return type_cast<T*>(this->base_t::ptr()); }
        constexpr const T* ptr() const  noexcept { return type_cast<const T*>(this->base_t::ptr()); }
        constexpr       T& ref()        noexcept { return *(this->self_t::ptr()); }
        constexpr const T& ref() const  noexcept { return *(this->self_t::ptr()); }
        constexpr T* operator->() noexcept { return this->self_t::ptr(); }
        constexpr const T* operator->() const noexcept { return this->self_t::ptr(); }
    };
} // namespace sia
