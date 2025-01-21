#pragma once

#include <bit>

#include "SIA/internals/types.hpp"
#include "SIA/utility/type_container.hpp"
#include "SIA/container/constant_string.hpp"

namespace sia
{
    namespace layout_detail
    {
        constexpr auto get_max_space = [] <typename... Fs> () constexpr noexcept -> size_t
            {
                constexpr auto comp = [] <typename T> (size_t& arg) constexpr noexcept -> void
                    {
                        T copy { };
                        if (arg > copy.get_space_size()) { arg = copy.get_space_size(); }
                    };
                size_t ret { };
                (comp.template operator()<Fs>(ret), ...);
                return ret;
            };

        template <typename T>
        struct layout_impl;

        template <typename... Ts>
        struct layout_impl<type_container<Ts...>>
        {
        private:
            using tcon_t = type_container<Ts...>;
            unsigned_interger_t<1> m_chunk[get_max_space.template operator()<>()];
        public:
            template <size_t Idx>
            constexpr auto& get() noexcept
            {
                typename tcon_t::template at_t<Idx> fr { };
                return *reinterpret_cast<typename tcon_t::template at_t<Idx>::target_t*>(m_chunk + fr.get_pos());
            }

            template <constant_string Str>
            constexpr auto& get() noexcept
            {
                return 0;
            }
        };
    } // namespace layout_detail

    template <size_t Pos, typename Type, constant_string Name>
    struct frame
    {
        using target_t = Type;
        constexpr size_t get_pos(this auto&& self) noexcept { return Pos; }
        constexpr decltype(Name) get_name(this auto&& self) noexcept { return Name; }
        constexpr size_t get_space_size(this auto&& self) noexcept { return Pos + sizeof(Type); }
    };

    template <typename... Fs>
    struct layout : public layout_detail::layout_impl<type_container<Fs...>>
    {
        
    };
} // namespace sia
