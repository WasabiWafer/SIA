#pragma once

#include <type_traits>
#include <span>
#include <chrono>
#include <array>

#include "SIA/internals/types.hpp"

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
        struct time_exp<tags::time_unit::nanoseconds, Rep> { using type = std::chrono::duration<Rep, std::nano>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::microseconds, Rep> { using type = std::chrono::duration<Rep, std::micro>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::milliseconds, Rep> { using type = std::chrono::duration<Rep, std::milli>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::seconds, Rep> { using type = std::chrono::duration<Rep>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::minutes, Rep> { using type = std::chrono::duration<Rep, std::ratio<60>>; };
        template <typename Rep>
        struct time_exp<tags::time_unit::hours, Rep> { using type = std::chrono::duration<Rep, std::ratio<3600>>; };

        template <tags::time_unit Tag, typename Rep = float>
        using time_exp_t = time_exp<Tag, Rep>::type;
    } // namespace recorder_detail
    
    template <size_t LoopNum, auto... Callables>
    struct constant_runner;

    struct single_recorder
    {
    private:
        template <size_t N, auto... Es>
        friend class constant_runner;
        using clock_t = std::chrono::high_resolution_clock;
        using tp_t = std::chrono::high_resolution_clock::time_point;
        tp_t m_record[2];
    
    public:
        void set() noexcept { m_record[0] = clock_t::now(); }
        void now() noexcept { m_record[1] = clock_t::now(); }
        template <tags::time_unit Unit, typename Rep = float>
        Rep reuslt() noexcept { return recorder_detail::time_exp_t<Unit, Rep>(m_record[1] - m_record[0]).count(); }
        std::span<tp_t, 2> result_span() noexcept { return m_record; }
    };    

    template <size_t LoopNum, auto... Callables>
    struct constant_runner
    {
    private:
        using tp_t = single_recorder::tp_t;
        using seq_t = std::make_index_sequence<sizeof...(Callables)>;
        template <typename Rep>
        using result_t = std::array<Rep, sizeof...(Callables)>;

        single_recorder m_sr;
        std::pair<tp_t, tp_t> m_record[sizeof...(Callables)];

        constexpr size_t loop_count() noexcept { return LoopNum; }
        constexpr size_t call_size() noexcept { return sizeof...(Callables); }

        template <typename T>
        constexpr void call_impl(size_t pos, T&& call) noexcept(noexcept(call.operator()()))
        {
            this->m_sr.set();
            for(size_t pos{ }; pos < this->loop_count(); ++pos)
            { call.operator()(); }
            this->m_sr.now();
            auto r_span = this->m_sr.result_span();
            this->m_record[pos] = {r_span[0], r_span[1]};
        }

        template <size_t... Seqs>
        constexpr void run_impl(std::index_sequence<Seqs...> seq = seq_t()) noexcept((noexcept(this->call_impl(Seqs, Callables)) && ...))
        { (this->call_impl(Seqs, Callables), ...); }

        template <tags::time_unit Tag, typename Rep>
        constexpr Rep get_nth_result(size_t pos) noexcept
        {
            std::pair<tp_t, tp_t>& target = this->m_record[pos];
            return recorder_detail::time_exp_t<Tag, Rep>(target.second - target.first);
        }

        template <tags::time_unit Tag, typename Rep, size_t... Seqs>
        constexpr result_t<Rep> result_impl(std::index_sequence<Seqs...> seq = seq_t()) noexcept(noexcept(result_t<Rep>{get_nth_result<Tag, Rep>(Seqs)...}))
        { return {get_nth_result<Tag, Rep>(Seqs)...}; }

    public:
        constexpr void run() noexcept(noexcept(this->run_impl()))
        { this->run_impl(); }

        template <tags::time_unit Tag = tags::time_unit::seconds, typename Rep = float>
        constexpr result_t<Rep> result() noexcept(noexcept(result_impl<Tag, Rep>()))
        { return result_impl<Tag, Rep>(); }
    };
} // namespace sia