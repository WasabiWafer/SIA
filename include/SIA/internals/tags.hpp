#pragma once

namespace sia
{
    namespace tags
    {
        enum class os { window, linux, other };
        enum class arch { x32, x64, other };
    } // namespace tag
} // namespace sia

namespace sia
{
    namespace tags_detail
    {
        constexpr auto get_os() noexcept
        {
            #if defined(_WIN32)
                #define SIA_OS_WINDOW
                return tags::os::window;
            #elif defined(__linux__)
                #define SIA_OS_LINUX
                return tags::os::linux;
            #else
                #define SIA_OS_OTHER
                return tags::os::other;
            #endif
        }

        constexpr auto get_architecture() noexcept
        {
            #if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
                #define SIA_ARCH_X64
                return tags::arch::x64;
            #elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
                #define SIA_ARCH_X32
                return tags::arch::x32;
            #else
                #define SIA_ARCH_OTHER
                return tags::arch::other;
            #endif
        }

        constexpr unsigned int get_os_bit() noexcept
        {
            #if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
                #define SIA_OS_BIT_64
                return 64;
            #elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
                #define SIA_OS_BIT_32
                return 32;
            #else
                return 0;
            #endif
        }
    } // namespace tags_detail
} // namespace sia


namespace sia
{
    namespace system
    {
        constexpr auto os = tags_detail::get_os();
        constexpr auto os_bit = tags_detail::get_os_bit();
        constexpr auto arch = tags_detail::get_architecture();
    };
} // namespace sia