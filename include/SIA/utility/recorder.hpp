#pragma once

#include <type_traits>
#include <span>
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
        template <tags::time_unit Unit, typename Rep = float>
        Rep reuslt() noexcept { return recorder_detail::time_exp_t<Unit, Rep>(m_record[1] - m_record[0]).count(); }
    };    

    template <typename Rep, size_t LoopNum, auto... Callables>
    struct constant_runner
    {
    private:
        using seq_t = std::make_index_sequence<sizeof...(Callables)>;
        using result_t = std::span<Rep, sizeof...(Callables)>;

        single_recorder m_sr;
        Rep m_result[sizeof...(Callables)];

        constexpr size_t loop_count() noexcept { return LoopNum; }
        constexpr size_t call_size() noexcept { return sizeof...(Callables); }

        template <tags::time_unit Unit, typename T>
        constexpr void call_impl(size_t pos, T&& call) noexcept(noexcept(call.operator()()))
        {
            this->m_sr.set();
            for(size_t pos{ }; pos < this->loop_count(); ++pos)
            { call.operator()(); }
            this->m_sr.now();
            this->m_result[pos] = this->m_sr.template reuslt<Unit, Rep>();
        }

        template <tags::time_unit Unit, size_t... Seqs>
        constexpr void run_impl(std::index_sequence<Seqs...> seq = seq_t()) noexcept((noexcept(this->call_impl<Unit>(Seqs, Callables)) && ...))
        { 
            // this->call_impl<Unit>(0, [](){});
            (this->call_impl<Unit>(Seqs, Callables), ...);
            // ((this->call_impl<Unit>(Seqs, [](){}), this->call_impl<Unit>(Seqs, Callables)) , ...); // better ?
        }

    public:
        template <tags::time_unit Tag = tags::time_unit::seconds>
        constexpr void run() noexcept(noexcept(this->run_impl<Tag>(seq_t())))
        { this->run_impl<Tag>(seq_t()); }

        result_t result() noexcept { return result_t(static_cast<Rep*>(this->m_result), sizeof...(Callables)); }
    };
} // namespace sia