#pragma once

#include <memory>
#include <atomic>

#include "SIA/utility/compressed_pair.hpp"
#include "SIA/utility/align_wrapper.hpp"
#include "SIA/container/tail.hpp"

// lock free spsc tail
namespace sia
{
    namespace concurrency
    {
        namespace lock_free
        {
            namespace spsc
            {             
                template <typename T, typename Allocator = std::allocator<T>>
                struct tail
                {

                };
            } // namespace mpmc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia

// lock free mpmc tail
namespace sia
{
    namespace concurrency
    {
        namespace lock_free
        {
            namespace mpmc
            {             
                template <typename T, typename Allocator = std::allocator<T>>
                struct tail
                {

                };
            } // namespace mpmc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
