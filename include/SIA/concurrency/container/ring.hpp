#pragma once

#include <memory>
#include <atomic>

#include "SIA/utility/align_wrapper.hpp"

namespace sia
{
    namespace concurrency
    {
        namespace lock_free
        {
            namespace spsc
            {
                namespace ring_detail
                {
                    template <typename T, size_t Size>
                    struct ring_composition
                    {
                        false_share<std::atomic<T*>> m_data;
                        false_share<std::atomic<size_t>> m_begin;
                        false_share<std::atomic<size_t>> m_end;
                    };
                } // namespace ring_detail
                
                template <typename T, size_t Size, typename Allocator = std::allocator<T>>
                struct ring
                {

                };
            } // namespace spsc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
