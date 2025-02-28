#pragma once

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/type_container.hpp"
#include "SIA/utility/type/constant_string.hpp"

namespace sia
{
    template <size_t BytePos, typename Type, constant_string Name>
    struct layout
    {
    public:
        using type = Type;
        constexpr size_t req_size(this auto&& self) noexcept { return BytePos + sizeof(Type); }
        constexpr auto name(this auto&& self) noexcept { return Name; }
        constexpr size_t pos(this auto&& self) noexcept { return BytePos; }
    };
    
    template <size_t Pos, typename Type, constant_string Name>
    using layout_t = layout<Pos, Type, Name>;

    namespace frame_detail
    {
        template <typename T>
        struct LayoutType
        { static constexpr bool value = false; };

        template <size_t BytePos, typename Type, constant_string Name>
        struct LayoutType<layout<BytePos, Type, Name>>
        { static constexpr bool value = true; };

        template <typename T>
        constexpr bool LayoutType_v = LayoutType<T>::value;

        template <typename... Ts>
        consteval size_t calc_layout_max_size() noexcept
        {
            size_t ret { };
            constexpr auto max = [] (size_t& arg, const size_t in) constexpr noexcept -> void { if(arg < in) { arg = in; } };
            (max(ret, Ts().req_size()), ...);
            return ret;
        }

        template <size_t ByteSize, typename... Ts>
        struct frame_impl
        {
        private:
            using tcon_t = type_container<Ts...>;
            using seq_t = std::make_index_sequence<sizeof...(Ts)>;

            chunk<byte_t, ByteSize> m_data;

            constexpr byte_t<1>* address(size_t pos) noexcept { return m_data.m_bin + pos; }

            template <size_t Nth>
            constexpr auto ptr() noexcept
            {
                using pos_t = tcon_t::template at_t<Nth>;
                constexpr pos_t obj { };
                return reinterpret_cast<pos_t::type*>(this->address(obj.pos()));
            }

            template <size_t Nth>
            constexpr const auto ptr() const noexcept
            {
                using pos_t = tcon_t::template at_t<Nth>;
                constexpr pos_t obj { };
                return reinterpret_cast<pos_t::type*>(this->address(obj.pos()));
            }

            template <size_t Nth>
            constexpr auto& ref() noexcept { return *this->template ptr<Nth>(); }

            template <size_t Nth>
            constexpr const auto& ref() const noexcept { return *this->template ptr<Nth>(); }

        public:
            template <size_t Nth>
                requires (Nth < sizeof...(Ts))
            constexpr auto& get(this auto&& self) noexcept { return self.template ref<Nth>(); }

            template <constant_string Str>
            constexpr auto& get(this auto&& self) noexcept
            {
                constexpr auto select_impl = [] <size_t Pos> (std::pair<bool, size_t>& arg) constexpr noexcept -> auto
                    {
                        using pos_t = tcon_t::template at_t<Pos>;
                        constexpr pos_t obj { };
                        if (obj.name().to_string_view().compare(Str.to_string_view()) == 0)
                        {
                            arg.first = true;
                            arg.second = Pos;
                        }
                    };
                constexpr auto select = [select_impl] <size_t... Pos>(std::index_sequence<Pos...> seq) constexpr noexcept -> auto
                    {
                        std::pair<bool, size_t> ret{ };
                        (select_impl.template operator()<Pos>(ret), ...);
                        return ret;
                    };
                constexpr auto wrap = [select] () constexpr noexcept -> size_t
                    {
                        constexpr auto result = select(seq_t{ });
                        static_assert(result.first, "error : request non exist object name.");
                        return result.second;
                    };
                return self.template get<wrap()>();
            }
        };
    } // namespace frame_detail

    template <typename... Ts>
        requires (frame_detail::LayoutType_v<Ts> && ...)
    struct frame : public frame_detail::frame_impl<frame_detail::calc_layout_max_size<Ts...>(), Ts...>
    { };
} // namespace sia