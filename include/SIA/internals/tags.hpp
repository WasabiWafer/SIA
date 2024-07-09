#pragma once

namespace sia
{
    namespace tags
    {
        enum class os { window, linux, other };
    } // namespace tag
} // namespace sia

namespace sia
{
    namespace tags_detail
    {
        constexpr auto get_os() noexcept
        {
            #if     defined(_WIN32)
            return tags::os::window;
            #elif   defined(__linux__)
            return tags::os::linux;
            #else
            return tags::os::other;
            #endif
        }
    } // namespace tags_detail
} // namespace sia


namespace sia
{
    namespace system
    {
        constexpr auto os = tags_detail::get_os();
    };
} // namespace sia