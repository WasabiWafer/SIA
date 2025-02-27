#pragma once

#include <type_traits>
#include <print>
#include <chrono>

namespace sia
{
    namespace tags
    {
        enum class time_unit { nanoseconds, microseconds, milliseconds, seconds, minutes, hours};
    } // namespace tags
    
    namespace recorder_detail
    {
        template <tags::time_unit Tag, typename Rep = float>
        struct time_exp;
        template <typename Rep>
        struct time_exp<tags::time_unit::nanoseconds, Rep> { using type = std::chrono::duration<float, std::nano>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::microseconds, Rep> { using type = std::chrono::duration<float, std::micro>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::milliseconds, Rep> { using type = std::chrono::duration<float, std::milli>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::seconds, Rep> { using type = std::chrono::duration<float>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::minutes, Rep> { using type = std::chrono::duration<float, std::ratio<60>>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::hours, Rep> { using type = std::chrono::duration<float, std::ratio<3600>>; };
        template <tags::time_unit Tag, typename Rep = float>
        using time_exp_t = time_exp<Tag, Rep>::type;
    } // namespace recorder_detail
    
    struct single_recorder
    {
    private:
        using clock_t = std::chrono::high_resolution_clock;
        using tp_t = std::chrono::high_resolution_clock::time_point;
        tp_t m_record[2];
        
    public:
        void set() noexcept { m_record[0] = clock_t::now(); }
        void now() noexcept { m_record[1] = clock_t::now(); }
        template <tags::time_unit Tag, typename Rep = float>
        auto reuslt() noexcept { return recorder_detail::time_exp_t<Tag, Rep>(m_record[1] - m_record[0]); }
    };
} // namespace sia