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
            else { return void{ }; }
        }

        template <size_t Size>
        consteval auto get_floating_type() noexcept
        {
            if      constexpr (Size == sizeof(float))       { using ret_t = float;          return ret_t{ }; }
            else if constexpr (Size == sizeof(double))      { using ret_t = double;         return ret_t{ }; }
            else if constexpr (Size == sizeof(long double)) { using ret_t = long double;    return ret_t{ }; }
            else { return void{ }; }
        }
    } // namespace types_detail

    using size_t = decltype(sizeof(void*));
    template <size_t Size> using signed_interger_t   = std::make_signed_t  <decltype(types_detail::get_integer_type<Size>())>;
    template <size_t Size> using unsigned_interger_t = std::make_unsigned_t<decltype(types_detail::get_integer_type<Size>())>;
    template <size_t Size> using floating_point_t    = decltype(types_detail::get_floating_type<Size>());
} // namespace sia

