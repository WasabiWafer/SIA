#pragma once

#include <memory>
#include <atomic>

#include "SIA/utility/align_wrapper.hpp"
#include "SIA/container/tail.hpp"

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
                private:
                    using allocator_traits_t = std::allocator_traits<Allocator>;
                    using tail_data_t = ::sia::tail_detail::tail_data<T>;

                    false_share<tail_data_t*> m_last;
                    ::sia::tail<T, Allocator> m_tail;
                public:
                    
                };
            } // namespace mpmc
        } // namespace lock_free
    } // namespace concurrency
} // namespace sia
