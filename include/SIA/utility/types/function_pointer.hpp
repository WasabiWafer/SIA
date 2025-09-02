#pragma once

#include <type_traits>

#include "SIA/internals/define.hpp"
#include "SIA/internals/types.hpp"

#include "SIA/utility/tools.hpp"

namespace sia
{
    namespace function_pointer_detail
    {
        template <bool IsMemFunc, bool IsNothrow, tags::type_qualifier ThisQualifier, tags::reference_category ThisReferenceCategory>
        struct function_flag
        {
            static constexpr bool member_function_flag = IsMemFunc;
            static constexpr bool nothrow_flag = IsNothrow;
            static constexpr tags::type_qualifier this_qualifier_tag = ThisQualifier;
            static constexpr tags::reference_category this_reference_category_tag = ThisReferenceCategory;
        };

        template <typename ReturnType, typename ClassType, typename... ArgsType>
        struct function_sig
        {
            using return_t = ReturnType;
            using class_t = ClassType;
            using arg_type_list_t = type_list<ArgsType...>;
        };

        template <typename Func>
        struct function_detail;

        // basic function begin
        template <typename ReturnType, typename... ArgTypes>
        struct function_detail<ReturnType (*)(ArgTypes...)> :
            function_flag<false, false, tags::type_qualifier::none, tags::reference_category::none>,
            function_sig<ReturnType, void, ArgTypes...>
        { };

        template <typename ReturnType, typename... ArgTypes>
        struct function_detail<ReturnType (*)(ArgTypes...) noexcept> :
            function_flag<false, true, tags::type_qualifier::none, tags::reference_category::none>,
            function_sig<ReturnType, void, ArgTypes...>
        { };
        // basic function end

        // member function begin
            //member function basic begin
            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...)> :
                function_flag<true, false, tags::type_qualifier::none, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const> :
                function_flag<true, false, tags::type_qualifier::const_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile> :
                function_flag<true, false, tags::type_qualifier::volatile_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile> :
                function_flag<true, false, tags::type_qualifier::const_volatile_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) noexcept> :
                function_flag<true, true, tags::type_qualifier::none, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const noexcept> :
                function_flag<true, true, tags::type_qualifier::const_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile noexcept> :
                function_flag<true, true, tags::type_qualifier::volatile_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile noexcept> :
                function_flag<true, true, tags::type_qualifier::const_volatile_type, tags::reference_category::none>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };
            //member function basic end

            //member function & begin
            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) &> :
                function_flag<true, false, tags::type_qualifier::none, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const &> :
                function_flag<true, false, tags::type_qualifier::const_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile &> :
                function_flag<true, false, tags::type_qualifier::volatile_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile &> :
                function_flag<true, false, tags::type_qualifier::const_volatile_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) & noexcept> :
                function_flag<true, true, tags::type_qualifier::none, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const & noexcept> :
                function_flag<true, true, tags::type_qualifier::const_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile & noexcept> :
                function_flag<true, true, tags::type_qualifier::volatile_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile & noexcept> :
                function_flag<true, true, tags::type_qualifier::const_volatile_type, tags::reference_category::lvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };
            //member function & end

            //member function && begin
            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) &&> :
                function_flag<true, false, tags::type_qualifier::none, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const &&> :
                function_flag<true, false, tags::type_qualifier::const_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile &&> :
                function_flag<true, false, tags::type_qualifier::volatile_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile &&> :
                function_flag<true, false, tags::type_qualifier::const_volatile_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) && noexcept> :
                function_flag<true, true, tags::type_qualifier::none, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const && noexcept> :
                function_flag<true, true, tags::type_qualifier::const_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) volatile && noexcept> :
                function_flag<true, true, tags::type_qualifier::volatile_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };

            template <typename ReturnType, typename ClassType, typename... ArgTypes>
            struct function_detail<ReturnType (ClassType::*)(ArgTypes...) const volatile && noexcept> :
                function_flag<true, true, tags::type_qualifier::const_volatile_type, tags::reference_category::rvalue>,
                function_sig<ReturnType, ClassType, ArgTypes...>
            { };
            //member function && end
        //member function end
    } // namespace function_pointer_detail
    
    template <typename FuncPtrType>
    using function_info_t = function_pointer_detail::function_detail<FuncPtrType>;

    template <typename FuncPtrType, typename ArgTypeList = function_info_t<FuncPtrType>::template arg_type_list_t>
    struct function_pointer;

    // for raw fp
    template <typename FuncPtrType, typename... ArgTypes>
        requires (std::is_pointer_v<FuncPtrType> && std::is_function_v<std::remove_pointer_t<FuncPtrType>>)
    struct function_pointer<FuncPtrType, type_list<ArgTypes...>> : public function_info_t<FuncPtrType>
    {
        private:
            FuncPtrType m_fp;

        public:
            using info_t = function_info_t<FuncPtrType>;
            constexpr function_pointer() noexcept = default;
            constexpr function_pointer(const function_pointer&) noexcept = default;
            constexpr function_pointer(FuncPtrType ptr) noexcept
                : m_fp(ptr)
            { }
            
            constexpr function_pointer& operator=(const function_pointer&) noexcept = default;
            constexpr function_pointer& operator=(FuncPtrType ptr) noexcept
            {
                m_fp = ptr;
                return *this;
            }

            constexpr info_t::return_t operator()(ArgTypes... args) noexcept(info_t::nothrow_flag)
            { return m_fp(std::forward<ArgTypes>(args)...); }
            
            constexpr info_t::return_t call(ArgTypes... args) noexcept(info_t::nothrow_flag)
            { return m_fp(std::forward<ArgTypes>(args)...); }
    };

    // for member fp
    template <typename FuncPtrType, typename... ArgTypes>
        requires (std::is_member_function_pointer_v<FuncPtrType>)
    struct function_pointer<FuncPtrType, type_list<ArgTypes...>> : public function_info_t<FuncPtrType>
    {
        private:
            FuncPtrType m_fp;

        public:
            using info_t = function_info_t<FuncPtrType>;
            constexpr function_pointer() noexcept = default;
            constexpr function_pointer(const function_pointer&) noexcept = default;
            constexpr function_pointer(FuncPtrType ptr) noexcept
                : m_fp(ptr)
            { }
            
            constexpr function_pointer& operator=(const function_pointer&) noexcept = default;
            constexpr function_pointer& operator=(FuncPtrType ptr) noexcept
            {
                m_fp = ptr;
                return *this;
            }            

            // member function begin
                // no_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t operator()(info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t operator()(const info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t operator()(volatile info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t operator()(const volatile info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                // lv_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(const info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(volatile info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(const volatile info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                // rv_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(const info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(volatile info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t operator()(const volatile info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }
            // member function end

            // member function begin
                // no_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t call(info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t call(const info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t call(volatile info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::none))
                constexpr info_t::return_t call(const volatile info_t::class_t* obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj->*m_fp)(std::forward<ArgTypes>(args)...); }

                // lv_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(const info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(volatile info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(const volatile info_t::class_t& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                // rv_ref
                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::none) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(const info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(volatile info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }

                template <typename T = void>
                    requires ((info_t::this_qualifier_tag == tags::type_qualifier::const_volatile_type) && (info_t::this_reference_category_tag == tags::reference_category::lvalue))
                constexpr info_t::return_t call(const volatile info_t::class_t&& obj, ArgTypes... args) noexcept(info_t::nothrow_flag)
                { return (obj.*m_fp)(std::forward<ArgTypes>(args)...); }
            // member function end
    };

    template <typename FuncPtrType>
        requires ((std::is_pointer_v<FuncPtrType> && std::is_function_v<std::remove_pointer_t<FuncPtrType>>) || std::is_member_function_pointer_v<FuncPtrType>)
    function_pointer(FuncPtrType&&) -> function_pointer<std::remove_reference_t<FuncPtrType>>;
} // namespace sia
