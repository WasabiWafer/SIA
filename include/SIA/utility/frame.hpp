#pragma once

#include <bit>
#include <algorithm>

#include "SIA/internals/types.hpp"
#include "SIA/utility/tools.hpp"
#include "SIA/utility/type_container.hpp"
#include "SIA/utility/type/constant_string.hpp"

namespace sia
{
    template <size_t BytePos, typename Type, constant_string Name>
    struct layout;

    namespace frame_detail
    {
        constexpr auto select_impl = [] <constant_string Str, typename TypeCon, size_t Pos> (std::pair<bool, size_t>& arg) constexpr noexcept -> auto
        {
            using pos_t = TypeCon::template at_t<Pos>;
            constexpr pos_t obj { };
            if (obj.name().to_string_view().compare(Str.to_string_view()) == 0)
            {
                arg.first = true;
                arg.second = Pos;
            }
        };
        constexpr auto select = [] <constant_string Str, typename TypeCon, size_t... Seqs> (std::index_sequence<Seqs...> seq) constexpr noexcept -> auto
        {
            std::pair<bool, size_t> ret{ };
            (select_impl.template operator()<Str, TypeCon, Seqs>(ret), ...);
            return ret;
        };
        constexpr auto select_wrap = [] <constant_string Str, typename TypeCon, size_t... Seqs> (std::index_sequence<Seqs...> arg) constexpr noexcept -> size_t
        {
            constexpr auto result = select.operator()<Str, TypeCon>(arg);
            static_assert(result.first, "error : request non exist object name.");
            return result.second;
        };

        template <typename T>
        struct LayoutType { static constexpr bool value = false; };
        template <size_t BytePos, typename Type, constant_string Name>
        struct LayoutType<layout<BytePos, Type, Name>> { static constexpr bool value = true; };
        template <typename T>
        constexpr bool LayoutType_v = LayoutType<T>::value;

        template <typename... Ts>
        consteval size_t proc_get_layout_max_size() noexcept
        {
            size_t ret { };
            constexpr auto max = [] (size_t& arg, const size_t in) constexpr noexcept -> void { if(arg < in) { arg = in; } };
            (max(ret, Ts().req_size()), ...);
            return ret;
        }

        template <size_t ByteSize, typename... Ts>
        struct frame_impl : public chunk<byte_t, ByteSize>
        {
        private:
            using self_t = frame_impl;
            using tcon_t = type_container<Ts...>;
            using base_t = chunk<byte_t, ByteSize>;
            using seq_t = std::make_index_sequence<sizeof...(Ts)>;
            
        public:
            template <size_t Nth>
            constexpr auto ptr() noexcept
            {
                using pos_t = tcon_t::template at_t<Nth>;
                constexpr pos_t obj { };
                return type_cast<typename pos_t::type*>(this->base_t::ptr(obj.pos()));
            }
            template <size_t Nth>
            constexpr const auto ptr() const noexcept
            {
                using pos_t = tcon_t::template at_t<Nth>;
                constexpr pos_t obj { };
                return type_cast<const typename pos_t::type*>(this->base_t::ptr(obj.pos()));
            }
            template <constant_string CStr>
            constexpr auto ptr() noexcept
            { return this->self_t::template ptr<select_wrap.operator()<CStr, tcon_t>(seq_t())>(); }
            template <constant_string CStr>
            constexpr const auto ptr() const noexcept
            { return this->self_t::template ptr<select_wrap.operator()<CStr, tcon_t>(seq_t())>(); }

            template <size_t Nth>
            constexpr auto& ref() noexcept { return *(this->self_t::template ptr<Nth>()); }
            template <size_t Nth>
            constexpr const auto& ref() const noexcept { return *(this->self_t::template ptr<Nth>()); }
            template <constant_string CStr>
            constexpr auto& ref() noexcept
            { return this->self_t::template ref<select_wrap.operator()<CStr, tcon_t>(seq_t())>(); }
            template <constant_string CStr>
            constexpr const auto& ref() const noexcept
            { return this->self_t::template ref<select_wrap.operator()<CStr, tcon_t>(seq_t())>(); }

            constexpr byte_t* begin() noexcept { return this->base_t::ptr(); }
            constexpr const byte_t* begin() const noexcept { return this->base_t::ptr(); }
            constexpr byte_t* end() noexcept { return this->base_t::ptr(ByteSize); }
            constexpr const byte_t* end() const noexcept { return this->base_t::ptr(ByteSize); }
        };
    } // namespace frame_detail

    template <size_t BytePos, typename Type, constant_string Name = "">
    struct layout
    {
        using type = Type;
        constexpr auto name(this auto&& self) noexcept { return Name; }
        constexpr size_t pos(this auto&& self) noexcept { return BytePos; }
        constexpr size_t type_size(this auto&& self) noexcept { return sizeof(type); }
        constexpr size_t req_size(this auto&& self) noexcept { return self.pos() + self.type_size(); }
    };
    template <size_t Pos, typename Type, constant_string Name = "">
    using layout_t = layout<Pos, Type, Name>;

    template <typename... Ts>
        requires (frame_detail::LayoutType_v<Ts> && ...)
    struct frame : public frame_detail::frame_impl<frame_detail::proc_get_layout_max_size<Ts...>(), Ts...>
    {
    // private:
    //     using base_t = frame_detail::frame_impl<frame_detail::proc_get_layout_max_size<Ts...>(), Ts...>;
    // public:
    //     template <size_t N, size_t... Seqs>
    //         requires (N <= frame_detail::proc_get_layout_max_size<Ts...>())
    //     constexpr void assign(const byte_t (&arr)[N]) noexcept
    //     {
    //         if (this->base_t::begin() != static_cast<const byte_t*>(arr))
    //         { std::ranges::copy_n(static_cast<const byte_t*>(arr), N, this->base_t::begin()); }
    //     }

    //     constexpr void assign(const byte_t* ptr, size_t size) noexcept
    //     {
    //         if (ptr != this->base_t::begin())
    //         { std::ranges::copy_n(ptr, size, this->base_t::begin()); }
    //     }
    };
} // namespace sia