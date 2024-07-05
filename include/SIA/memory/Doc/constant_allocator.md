```cpp
template <auto E>
constexpr int baz()
{
    return E + E;
}

constexpr void bar(const int& arg)
{
    baz<arg>(); // error
}

template <auto E>
constexpr void foo(sia::constant_allocator<E> arg)
{
    baz<arg.alloc()>(); // fine
}

int main()
{
    sia::constant_allocator<10> ca { };
    foo(ca); // we can pass argument as constexpr with conallo(constant_allocator)
    bar(10); // error
    return 0;
}
```