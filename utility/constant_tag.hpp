#pragma once

#include <type_traits>

#include "SIA/utility/tools.hpp"

namespace sia
{
    template <auto... Ds>
        requires (std::is_scoped_enum_v<decltype(Ds)> && ...)
    struct constant_tag
    {
        template <typename... Ts>
            requires (std::is_scoped_enum_v<Ts> && ...)
        [[nodiscard]] constexpr bool query(Ts... args) noexcept
        {
            constexpr auto comp = overload {
                [] <typename T>                 (T  arg1, T  arg2) constexpr noexcept -> bool { return arg1 == arg2; },
                [] <typename T1, typename T2>   (T1 arg1, T2 arg2) constexpr noexcept -> bool { return false; }
            };
            constexpr auto run = [comp] (auto arg) constexpr noexcept -> bool {return (comp(Ds, arg) || ...);};
            return (run(args) || ...);
        }
    };
} // namespace sia
