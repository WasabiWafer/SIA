#pragma once

#include <type_traits>

namespace sia
{
    namespace types_detail
    {
        template <size_t Size>
        consteval auto get_integer_type() noexcept
        {
            if      constexpr (Size == sizeof(char))            { using ret_t = char;           return ret_t{ }; }
            else if constexpr (Size == sizeof(short int))       { using ret_t = short int;      return ret_t{ }; }
            else if constexpr (Size == sizeof(int))             { using ret_t = int;            return ret_t{ }; }
            else if constexpr (Size == sizeof(long int))        { using ret_t = long int;       return ret_t{ }; }
            else if constexpr (Size == sizeof(long long int))   { using ret_t = long long int;  return ret_t{ }; }
            else { return ; }
        }

        template <size_t Size>
        consteval auto get_floating_type() noexcept
        {
            if      constexpr (Size == sizeof(float))       { using ret_t = float;          return ret_t{ }; }
            else if constexpr (Size == sizeof(double))      { using ret_t = double;         return ret_t{ }; }
            else if constexpr (Size == sizeof(long double)) { using ret_t = long double;    return ret_t{ }; }
            else { return ; }
        }

        template <typename T0, typename T1>
        constexpr auto comp_leeq_ret_large() noexcept
        {
            if constexpr (sizeof(T0) <= sizeof(T1))
            { return T1{ }; }
            else { return T0{ }; }
        }
        
        template <typename T0, typename T1>
        constexpr auto comp_leeq_ret_small() noexcept
        {
            if constexpr (sizeof(T0) <= sizeof(T1))
            { return T0{ }; }
            else { return T1{ }; }
        }
    } // namespace types_detail

    using size_t = decltype(sizeof(void*));
    using largest_integer_t = long long int;
    using largest_floating_point_t = long double;
    using smallest_integer_t = char;
    using smallest_floating_point_t = float;
    template <size_t Size> using signed_interger_t   = std::make_signed_t  <decltype(types_detail::get_integer_type<Size>())>;
    template <size_t Size> using unsigned_interger_t = std::make_unsigned_t<decltype(types_detail::get_integer_type<Size>())>;
    template <size_t Size> using floating_point_t    = decltype(types_detail::get_floating_type<Size>());
    using largest_size_t = decltype(types_detail::comp_leeq_ret_large<largest_integer_t, largest_floating_point_t>());
    using smallest_size_t = decltype(types_detail::comp_leeq_ret_small<smallest_integer_t, smallest_floating_point_t>());
} // namespace sia

