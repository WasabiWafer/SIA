#pragma once

#include <array>

#include "SIA/internals/types.hpp"
#include "SIA/utility/type_container.hpp"
#include "SIA/container/constant_string.hpp"

namespace sia
{
    namespace layout_detail
    {
        template <typename T>
        struct layout_impl
        {

        };
    } // namespace layout_detail
    
    template <size_t Pos, typename Type, constant_string Name>
    struct frame { };

    template <typename... Fs>
    struct layout : private layout_detail::layout_impl<type_container<Fs...>>
    {
  
    };
} // namespace sia
