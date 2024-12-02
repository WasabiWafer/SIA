#pragma once

namespace sia
{
    namespace constant_allocator_detail
    {
        template <auto E> constexpr auto make_data = [] () constexpr noexcept -> decltype(E) { return E; };
        template <auto E> using make_data_t = decltype(make_data<E>);
    } // namespace constant_allocator_detail
    
    template <auto E>
    struct constant_allocator : private constant_allocator_detail::make_data_t<E>
    {
    private:
        using base_t = constant_allocator_detail::make_data_t<E>;
    public:
        constexpr auto allocate(this auto&& self) noexcept { return self.base_t::operator()(); }
        constexpr auto callable(this auto&& self) noexcept { return static_cast<base_t>(self); }
    };
}   //  namespace sia
