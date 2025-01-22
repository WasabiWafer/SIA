#pragma once

#include <string>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/type_container.hpp"
#include "SIA/utility/type/constant_string.hpp"

namespace sia
{

    namespace layout_detail
    {
        enum class unique { };

        constinit auto get_max_space = [] <typename... Fs> () constexpr noexcept -> size_t
            {
                constexpr auto comp = [] <typename T> (size_t& arg) constexpr noexcept -> void
                    {
                        T copy { };
                        if (arg < copy.space_size()) { arg = copy.space_size(); }
                    };
                size_t ret { };
                (comp.template operator()<Fs>(ret), ...);
                return ret;
            };

        template <typename T>
        struct layout_impl;

        template <typename... Ts>
        struct layout_impl<type_container<Ts...>> : public chunk<unsigned_interger_t<1>, get_max_space.template operator()<Ts...>()>
        {
        private:
            using tcon_t = type_container<Ts...>;
            template <size_t Idx>
            using target_frame = tcon_t::template at_t<Idx>;
            template <size_t Idx>
            using target_type = target_frame<Idx>::target_t;

        public:
            template <size_t Idx>
            constexpr auto& get(this auto&& self) noexcept
            {
                target_frame<Idx> fr { };
                return *reinterpret_cast<target_type<Idx>*>(self.m_bin + fr.pos());
            }

            template <constant_string CStr>
            constexpr auto& get(this auto&& self) noexcept
            {
                constexpr auto get_idx = [] <size_t Indx> (std::pair<bool, size_t>& arg) constexpr noexcept
                    {
                        target_frame<Indx> fr { };
                        if (CStr.to_string_view() == fr.name_sv())
                        {
                            arg.first = true;
                            arg.second = Indx;
                        }
                    };
                constexpr auto result = [get_idx] <size_t... Indxs> (std::index_sequence<Indxs...>) constexpr noexcept
                    {
                        std::pair<bool, size_t> ret { };
                        (get_idx.template operator()<Indxs>(ret), ...);
                        return ret;
                    };
                constexpr std::pair<bool, size_t> ret = result(std::make_index_sequence<sizeof...(Ts)>());
                static_assert(ret.first, "Error : Target CSTR Not Exist.");
                return self.template get<ret.second>();
            }
        };
    } // namespace layout_detail

    template <size_t Pos, typename Type, constant_string Name = "">
    struct frame
    {
        using target_t = Type;
        constexpr decltype(Name)    name(this auto&& self)      noexcept { return Name; }
        constexpr std::string       name_str(this auto&& self)  noexcept { return Name.to_string(); }
        constexpr std::string_view  name_sv(this auto&& self)   noexcept { return Name.to_string_view(); }
        constexpr size_t pos(this auto&& self) noexcept { return Pos; }
        constexpr size_t space_size(this auto&& self) noexcept { return Pos + sizeof(Type); }
        constexpr layout_detail::unique verify() noexcept { return layout_detail::unique(); }
    };

    template <typename... Fs>
        requires is_same_all_v<layout_detail::unique, decltype(std::declval<Fs>().verify())...>
    struct layout : public layout_detail::layout_impl<type_container<Fs...>>
    {
        
    };
} // namespace sia
