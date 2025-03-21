#pragma once

#include "SIA/internals/types.hpp"
#include "SIA/internals/define.hpp"

// not finish

namespace sia
{
    template <typename T>
    struct state_wrapper
    {
    private:
        tags::object_state m_state;
        T* m_object_ptr;
    
    public:
        constexpr state_wrapper() noexcept
            : m_state(tags::object_state::occupy), m_object_ptr(nullptr)
        { }

        constexpr state_wrapper(T* arg, const tags::object_state state = tags::object_state::allocated) noexcept
            : m_state(state), m_object_ptr(arg)
        { }

        
    };
} // namespace sia
