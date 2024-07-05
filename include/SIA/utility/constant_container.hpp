#pragma once

#include "SIA/utility/tools.hpp"
#include "SIA/memory/constant_allocator.hpp"

// constnat_sequence
namespace sia
{
    template <auto... Es>
    struct constant_sequence
    {

    };

    template <auto Begin, auto Last, auto Callable>
    constexpr auto foo() noexcept
    {
        // return entity_list<first Res...>
    }


    template <auto Begin, auto Last, auto Callable>
    constexpr auto make_con_seq = 0;
} // namespace sia

// constnat_container
namespace sia
{
    
} // namespace sia