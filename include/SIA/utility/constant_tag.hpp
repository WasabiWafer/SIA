#pragma once

#include <type_traits>

#include "SIA/utility/tools.hpp"

namespace sia
{
    template <auto... Ds> requires (std::is_scoped_enum_v<decltype(Ds)> && ...)
    struct constant_tag
    {
        template <typename... Cs> requires (std::is_scoped_enum_v<Cs> && ...)
        [[nodiscard]] constexpr bool query(this auto&& self, Cs... args) noexcept
        {
            constexpr auto comp = overload {
                [] <typename T>                 (T  arg)           constexpr noexcept -> bool { return false; },
                [] <typename T1, typename T2>   (T1 arg1, T2 arg2) constexpr noexcept -> bool { return false; },
                [] <typename T>                 (T  arg1, T  arg2) constexpr noexcept -> bool { return arg1 == arg2; }
            };
            constexpr auto run = [comp] (auto arg) constexpr noexcept -> bool {return (comp(arg, Ds) || ...);};
            return (run(args) || ...);
        }
        template <typename C>
        constexpr bool operator==(C arg) noexcept { return query(arg); }
        template <typename C>
        constexpr bool operator!=(C arg) noexcept { return !query(arg); }
    };
} // namespace sia
