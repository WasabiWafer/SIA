#pragma once

#include "SIA/utility/types/constant_string.hpp"
#include "SIA/utility/type_index.hpp"

namespace sia
{
    template <typename Type, constant_string Name, size_t BytePos>
    struct layout
    {
        template <typename Ty>
            requires (std::is_same_v<Ty, Type>)
        static constexpr Type* map(byte_t* base) noexcept
        { return type_cast<Type*>(base + BytePos); }
        template <typename Ty>
            requires (std::is_same_v<Ty, Type>)
        static constexpr const Type* map(const byte_t* base) noexcept
        { return type_cast<const Type*>(base + BytePos); }

        template <constant_string TargetName>
            requires (TargetName == Name)
        static constexpr Type* map(byte_t* base) noexcept
        { return type_cast<Type*>(base + BytePos); }
        template <constant_string TargetName>
            requires (TargetName == Name)
        static constexpr const Type* map(const byte_t* base) noexcept
        { return type_cast<const Type*>(base + BytePos); }

        static constexpr Type* get(byte_t* base) noexcept
        { return type_cast<Type*>(base + BytePos); }
        static constexpr const Type* get(const byte_t* base) noexcept
        { return type_cast<const Type*>(base + BytePos); }

        static constexpr size_t require_size() noexcept { return sizeof(Type) + BytePos; }
    };

    namespace frame_detail
    {
        template <typename T>
        struct is_layout_type : std::bool_constant<false> { };
        template <typename T, constant_string Name, size_t BytePos>
        struct is_layout_type<layout<T, Name, BytePos>> : std::bool_constant<true> { };
    
        SIA_MACRO_GEN_OVERLOAD(map_overload, map)

        template <typename T>
        constexpr void calc_requires_size_impl(size_t& arg) noexcept
        {
            if (arg < T::require_size())
            { arg = T::require_size(); }
        }
    
        template <typename... Ts>
        constexpr size_t calc_requires_size() noexcept
        {
            size_t ret { };
            (calc_requires_size_impl<Ts>(ret), ...);
            return ret;
        }
    } // namespace frame_detail
    


    template <typename... Ts>
        requires (frame_detail::is_layout_type<Ts>::value && ...)
    struct frame : public chunk<byte_t, frame_detail::calc_requires_size<Ts...>()>
    {
        private:
            using base_t = chunk<byte_t, frame_detail::calc_requires_size<Ts...>()>;
            using type_index_t = type_index<Ts...>;
            static constexpr const auto mapper = frame_detail::map_overload { Ts{ }... };

        public:
            template <typename TargetType>
            constexpr auto*         ptr()       noexcept
            { return mapper.map<TargetType>(this->base_t::ptr()); }
            template <typename TargetType>
            constexpr const auto*   ptr() const noexcept
            { return mapper.map<TargetType>(this->base_t::ptr()); }

            template <constant_string TargetString>
            constexpr auto*         ptr()       noexcept
            { return mapper.map<TargetString>(this->base_t::ptr()); }
            template <constant_string TargetString>
            constexpr const auto*   ptr() const noexcept
            { return mapper.map<TargetString>(this->base_t::ptr()); }

            template <size_t N>
            constexpr auto*         ptr()       noexcept
            { return type_index_t::at<N>::get(this->base_t::ptr()); }
            template <size_t N>
            constexpr const auto*   ptr() const noexcept
            { return type_index_t::at<N>::get(this->base_t::ptr()); }

            template <typename TargetType>
            constexpr auto&         ref()       noexcept
            { return *this->ptr<TargetType>(); }
            template <typename TargetType>
            constexpr const auto&   ref() const noexcept
            { return *this->ptr<TargetType>(); }

            template <constant_string TargetString>
            constexpr auto&         ref()       noexcept
            { return *this->ptr<TargetString>(); }
            template <constant_string TargetString>
            constexpr const auto&   ref() const noexcept
            { return *this->ptr<TargetString>(); }

            template <size_t N>
            constexpr auto&         ref()       noexcept
            { return *this->ptr<N>(); }
            template <size_t N>
            constexpr const auto&   ref() const noexcept
            { return *this->ptr<N>(); }
    };

    namespace frame_detail
    {
        template <size_t Idx, typename... Ts>
            requires (Idx > 0)
        constexpr size_t get_byte_pos(sia::type_index<Ts...>) noexcept
        {
            size_t ret { };
            ((ret += sizeof(Ts)), ...);
            return ret;
        }

        template <size_t Idx, typename... Ts>
            requires (Idx == 0)
        constexpr size_t get_byte_pos(sia::type_index<Ts...>) noexcept
        { return 0; }

        template <typename T, size_t Idx, typename... Ts>
            requires (Idx > 0)
        constexpr auto gen_layout(sia::type_index<Ts...>) noexcept
        {
            using index_t = sia::type_index<Ts...>;
            using select_index_t = index_t::template select<0, Idx>;
            return sia::layout<T, "", get_byte_pos<Idx>(select_index_t{ })>{ };
        }

        template <typename T, size_t Idx, typename... Ts>
            requires (Idx == 0)
        constexpr auto gen_layout(sia::type_index<Ts...>) noexcept
        { return sia::layout<T, "", 0>{ }; }

        template <typename... Ts, size_t... Seqs>
        constexpr auto gen_frame(std::index_sequence<Seqs...> seq) noexcept
        {
            using index_t = sia::type_index<Ts...>;
            return sia::frame<decltype(gen_layout<Ts, Seqs>(index_t{ }))...>{ };
        }

        template <typename... Ts>
        struct tuple_frame_impl;

        template <typename... Ts, size_t... Seqs>
        struct tuple_frame_impl<std::index_sequence<Seqs...>, Ts...> : decltype(gen_frame<Ts...>(std::make_index_sequence<sizeof...(Ts)>{ }))
        {
            private:
                using base_t = decltype(gen_frame<Ts...>(std::make_index_sequence<sizeof...(Ts)>{ }));
            public:
                constexpr tuple_frame_impl() noexcept = default;
                constexpr tuple_frame_impl(const Ts&... args)
                    noexcept((std::is_nothrow_constructible_v<Ts, const Ts&> && ...))
                    : base_t()
                { ((this->base_t::template ref<Seqs>() = args), ...); }
        };
    } // namespace frame_detail


    template <typename... Ts>
    struct tuple_frame : public frame_detail::tuple_frame_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {
        private:
            using base_t = frame_detail::tuple_frame_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
        public:
            constexpr tuple_frame() noexcept = default;
            constexpr tuple_frame(const Ts&... args) 
                noexcept((std::is_nothrow_constructible_v<Ts, const Ts&> && ...))
                : base_t(args...)
            { }

            // maybe intellisense can't deducing 'frame' type.
            // this function could help no error(not actually error) / convenient use.
            template <size_t N>
            constexpr typename auto& get() noexcept { return this->base_t::template ref<N>(); }
    };
} // namespace sia