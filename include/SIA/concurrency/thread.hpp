#pragma once

#include "SIA/concurrency/declare.h"
#include "SIA/internals/types.hpp"
#include "SIA/internals/tags.hpp"
#include "SIA/utility/constant_tag.hpp"

namespace sia
{
    namespace thread_detail
    {
        
    } // namespace thread_detail
    
    template <auto... Es>
    struct thread;
    template <>
    struct thread<system::os> : private constant_tag<system::os>
    {
        using tag_t = constant_tag<system::os>;
        
    };
} // namespace sia
