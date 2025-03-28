#pragma once

#include <stdfloat>

namespace sia
{
    using size_t = decltype(sizeof(void*));

    using largest_integer_t = long long int;
    using smallest_integer_t = char;

    using largest_unsigned_integer_t = unsigned long long int;
    using smallest_unsigned_integer_t = unsigned char;

    using largest_floating_point_t = long double;
    using smallest_floating_point_t = float;

    namespace types_detail
    {
        template <size_t Size>
        consteval auto get_integer_type() noexcept
        {
            if      constexpr (Size == sizeof(smallest_integer_t))  { return smallest_integer_t{ }; }
            else if constexpr (Size == sizeof(char))                { using ret_t = char;           return ret_t{ }; }
            else if constexpr (Size == sizeof(short int))           { using ret_t = short int;      return ret_t{ }; }
            else if constexpr (Size == sizeof(int))                 { using ret_t = int;            return ret_t{ }; }
            else if constexpr (Size == sizeof(long int))            { using ret_t = long int;       return ret_t{ }; }
            else if constexpr (Size == sizeof(long long int))       { using ret_t = long long int;  return ret_t{ }; }
            else if constexpr (Size == sizeof(largest_integer_t))   { return largest_integer_t{ }; }
            else { return nullptr; }
        }

        template <size_t Size>
        consteval auto get_unsigned_integer_type() noexcept
        {
            if      constexpr (Size == sizeof(smallest_unsigned_integer_t)) { return smallest_unsigned_integer_t{ }; }
            else if constexpr (Size == sizeof(unsigned char))               { using ret_t = unsigned char;           return ret_t{ }; }
            else if constexpr (Size == sizeof(unsigned short int))          { using ret_t = unsigned short int;      return ret_t{ }; }
            else if constexpr (Size == sizeof(unsigned int))                { using ret_t = unsigned int;            return ret_t{ }; }
            else if constexpr (Size == sizeof(unsigned long int))           { using ret_t = unsigned long int;       return ret_t{ }; }
            else if constexpr (Size == sizeof(unsigned long long int))      { using ret_t = unsigned long long int;  return ret_t{ }; }
            else if constexpr (Size == sizeof(largest_unsigned_integer_t))  { return largest_unsigned_integer_t{ }; }
            else { return nullptr; }
        }

        template <size_t Size>
        consteval auto get_floating_type() noexcept
        {
            if      constexpr (Size == sizeof(smallest_floating_point_t))   { return smallest_floating_point_t{ }; }
            else if constexpr (Size == sizeof(float))                       { using ret_t = float;      return ret_t{ }; }
            else if constexpr (Size == sizeof(double))                      { using ret_t = double;     return ret_t{ }; }
            else if constexpr (Size == sizeof(long double))                 { using ret_t = long double;return ret_t{ }; }
            else if constexpr (Size == sizeof(largest_floating_point_t))    { return largest_floating_point_t{ }; }
            else { return nullptr; }
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

    template <size_t Size> using signed_integer_t   = decltype(types_detail::get_integer_type<Size>());
    template <size_t Size> using unsigned_integer_t = decltype(types_detail::get_unsigned_integer_type<Size>());
    template <size_t Size> using floating_point_t    = decltype(types_detail::get_floating_type<Size>());
    using largest_size_t    = decltype(types_detail::comp_leeq_ret_large<largest_integer_t, largest_floating_point_t>());
    using smallest_size_t   = decltype(types_detail::comp_leeq_ret_small<smallest_integer_t, smallest_floating_point_t>());
    using byte_t    = unsigned_integer_t<1>;
    using word_t    = unsigned_integer_t<2>;
    using dword_t   = unsigned_integer_t<4>;
    using qword_t   = unsigned_integer_t<8>;
} // namespace sia

