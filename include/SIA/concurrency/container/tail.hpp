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
                namespace tail_datail
                {
                    template <typename T>
                    struct tail_data {
                    private:
                        T m_data;
                        false_share<std::atomic<tail_data*>> m_tail;
                    public:
                        constexpr T& data(this auto&& self) noexcept { return self.m_data; }
                        constexpr false_share<std::atomic<tail_data*>>& tail(this auto&& self) noexcept { return self.m_tail; }
                    };

                    template <typename T>
                    struct point {
                        tail_data<T>* back;
                    };
                } // namespace tail_datail
                
                template <typename T, typename Allocator = std::allocator<T>>
                struct tail {
                private:
                    using point_t = tail_detail::template point<T>;
                    using tail_data_t = tail_datail::tail_data<T>;
                    using alloc_rebind = std::allocator_traits<Allocator>::template rebind_alloc<tail_data_t>;
                    
                    tail_data_t* m_back;
                    compressed_pair<alloc_rebind, tail_data_t*> compair;
                public:
                    constexpr tail(const Allocator& alloc) noexcept
                        : m_back(nullptr), compair(compressed_pair_tag::one, alloc)
                    { }

                    constexpr tail_data_t* begin() noexcept { return compair.second(); }
                    constexpr tail_data_t* end() noexcept { return nullptr; }

                    template <typename... Cs>
                    constexpr void emplace_back(Cs&&... args) noexcept
                    {
                        constexpr auto rlx = std::memory_order_relaxed;
                        if (m_back == nullptr)
                        {
                            
                        }
                    }
                }; // struct tail
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
