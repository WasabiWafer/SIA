#pragma once

namespace sia
{
    namespace splits
    {
        #define SIA_MACRO_SPLITS_GEN_01(name) enum class name##_t { }; constexpr name##_t name##_v = name##_t{ };
        #define SIA_MACRO_SPLITS_GEN_05(x0, x1, x2, x3, x4) SIA_MACRO_SPLITS_GEN_01(x0) SIA_MACRO_SPLITS_GEN_01(x1) SIA_MACRO_SPLITS_GEN_01(x2) SIA_MACRO_SPLITS_GEN_01(x3) SIA_MACRO_SPLITS_GEN_01(x4)
        #define SIA_MACRO_SPLITS_GEN_10(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9) SIA_MACRO_SPLITS_GEN_05(x0, x1, x2, x3, x4) SIA_MACRO_SPLITS_GEN_05(x5, x6, x7, x8, x9)
        #define SIA_MACRO_SPLITS_GEN_20(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) SIA_MACRO_SPLITS_GEN_10(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9) SIA_MACRO_SPLITS_GEN_10(x10, x11, x12, x13, x14, x15, x16, x17, x18, x19)
        SIA_MACRO_SPLITS_GEN_10(zero, one, two, three, four, five, six, seven, eight, nine)
        SIA_MACRO_SPLITS_GEN_20(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t)
        SIA_MACRO_SPLITS_GEN_05(u, v, w, x, y)
        SIA_MACRO_SPLITS_GEN_01(z)
        SIA_MACRO_SPLITS_GEN_20(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T)
        SIA_MACRO_SPLITS_GEN_05(U, V, W, X, Y)
        SIA_MACRO_SPLITS_GEN_01(Z)
        #undef SIA_MACRO_SPLITS_GEN01
        #undef SIA_MACRO_SPLITS_GEN05
        #undef SIA_MACRO_SPLITS_GEN10
        #undef SIA_MACRO_SPLITS_GEN20
    } // namespace partition
} // namespace sia

namespace sia
{
    namespace tags
    {
        enum class numbers { zero, one, two, three, four, five, six, seven, eight, nine };
        enum class lower_letters { a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z };
        enum class upper_letters { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
        enum class memory_locations { stack, heap };
        enum class life_cycle { allocated, constructed, destructed, deallocated };
        enum class object_state { occupy, allocating, allocated, constructing, constructed, destructing, destructed, vacate };
        enum class os { windows, linux, other };
        enum class arch { x32, x64, other };
    } // namespace tag
} // namespace sia

namespace sia
{
    namespace stamps_detail
    {
        constexpr auto get_os() noexcept
        {
            #if defined(_WIN32)
                #define SIA_OS_WINDOWS
                return tags::os::windows;
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

        constexpr unsigned long long int get_os_bit() noexcept
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
    } // namespace stamps_detail
} // namespace sia


namespace sia
{
    namespace stamps
    {
        namespace system
        {
            constexpr const auto os_v = stamps_detail::get_os();
            constexpr const auto os_bit_v = stamps_detail::get_os_bit();
            constexpr const auto arch_v = stamps_detail::get_architecture();
        } // namespace system        
    } // namespace stamps
} // namespace sia