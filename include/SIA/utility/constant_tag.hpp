#pragma once

#include <type_traits>

#include "SIA/utility/tools.hpp"

namespace sia
{
    template <auto... Tags>
        requires (std::is_scoped_enum_v<decltype(Tags)> && ...)
    struct constant_tag
    {
        template <typename... Tys>
            requires (std::is_scoped_enum_v<Tys> && ...)
        constexpr bool query(this auto&& self, Tys... args) noexcept
        {
            constexpr static auto comp = overload {
                [] <typename T>                 (T  arg)           constexpr noexcept -> bool { return false; },
                [] <typename T1, typename T2>   (T1 arg1, T2 arg2) constexpr noexcept -> bool { return false; },
                [] <typename T>                 (T  arg1, T  arg2) constexpr noexcept -> bool { return arg1 == arg2; }
            };
            constexpr static auto run = [] (auto arg) constexpr noexcept -> bool { return (comp(arg, Tags) || ...); };
            return (run(args) || ...);
        }
        
        template <typename Ty>
            requires (std::is_scoped_enum_v<Ty>)
        constexpr bool operator==(this auto&& self, Ty arg) noexcept { return self.query(arg); }

        template <typename Ty>
            requires (std::is_scoped_enum_v<Ty>)
        constexpr bool operator!=(this auto&& self, Ty arg) noexcept { return !self.query(arg); }
    };
} // namespace sia
