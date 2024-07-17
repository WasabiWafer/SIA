#pragma once

#include <memory>

#include "SIA/internals/types.hpp"

namespace sia
{
    template <typename T, typename Allocator = std::allocator<T>>
    struct ring : private Allocator
    {
    private:
        size_t m_cap;
        T* m_ptr;

    public:
        constexpr ring() noexcept : m_cap(), m_ptr(nullptr) { }
        constexpr ring(size_t size) noexcept : m_cap(size), m_ptr(this->Allocator::allocate(size)) { new (m_ptr) T[m_cap](); }
        constexpr ring(const ring& arg) noexcept : m_cap(arg.m_cap), m_ptr(this->Allocator::allocate(arg.m_cap)) { std::memcpy(m_ptr, arg.m_ptr, sizeof(T) * arg.m_cap); }
        constexpr ring(ring&& arg) noexcept : m_cap(arg.m_cap), m_ptr(arg.m_ptr) { arg.m_ptr = nullptr; }
        constexpr ring& operator=(const ring& arg) noexcept
        {
            if (m_cap != arg.m_cap)
            {
                this->Allocator::deallocate(m_ptr, m_cap);
                m_ptr = this->Allocator::allocate(arg.m_cap);
            }
            std::memcpy(m_ptr, arg.m_ptr, sizeof(T) * m_cap);
            return *this;
        }
        constexpr ring& operator=(ring&& arg) noexcept
        {
            if (m_cap != arg.m_cap)
            {
                this->Allocator::deallocate(m_ptr, m_cap);
                m_ptr = this->Allocator::allocate(arg.m_cap);
            }
            m_ptr = arg.m_ptr;
            arg.m_ptr = nullptr;
            return *this;
        }
        ~ring() { this->Allocator::deallocate(m_ptr, m_cap); }

        constexpr size_t capacity(this auto&& self) noexcept { return self.m_cap; }
        constexpr T* data(this auto&& self) noexcept { return self.m_ptr; }
        constexpr T* begin(this auto&& self) noexcept { return self.data; }
        constexpr T* end(this auto&& self) noexcept { return self.data + self.m_cap; }
        constexpr T& operator[](this auto&& self, size_t pos) noexcept { return self.m_ptr[pos % self.m_cap]; }
        constexpr T* address(this auto&& self, size_t pos) noexcept { return self.m_ptr + (pos % self.m_cap); }
    };
} // namespace sia
