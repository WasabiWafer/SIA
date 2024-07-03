#pragma once

#include <memory>
#include <array>
#include <vector>

#include "SIA/internals/types.hpp"

// WIP : add hit, miss, fit func algorithm

namespace sia
{
    namespace memory_manager_detail
    {
        template <size_t Size>
        struct memory_block
        {
            using byte_t = unsigned_interger_t<1>;
            std::array<byte_t, Size> block;
        };

        template <size_t PageSize, size_t BlockSize>
        struct memory_page
        {
            std::array<memory_block<BlockSize>, PageSize> page;
        };
    } // namespace momory_manager_detail
    
    template <size_t InitSize, size_t PageSize, size_t BlockSize>
    struct memory_manager
    {
        using page_t = memory_manager_detail::memory_page<PageSize, BlockSize>;
        constexpr memory_manager() : memory {InitSize} { }
        std::vector<page_t> memory;
    };
} // namespace sia
