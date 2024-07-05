#pragma once

namespace sia
{
    namespace constant_allocator_detail
    {
        template <auto Data> constexpr auto make_data = [] () constexpr noexcept -> decltype(Data) { return Data; };
        template <auto Data> using make_data_t = decltype(make_data<Data>);
    } // namespace constant_allocator_detail
    
    template <auto Data>
    struct constant_allocator : private constant_allocator_detail::make_data_t<Data>
    {
    private:
        using base_t = constant_allocator_detail::make_data_t<Data>;
    public:
        constexpr auto alloc() noexcept { return base_t::operator()(); }
        constexpr auto callable() noexcept { return base_t{ }; }
    };
}   //  namespace sia
