# Type Container
type_container is offer several functionally of manage template type parameter pack.  
```cpp
int main()
{
    constexpr auto lam0 = [] <typename T> () { return std::is_same_v<typename T::contain_t, double>; };
    // input is type_pair<Key, Type>, type_pair<Key, Type>::contain_t == Type

    sia::type_container<char, short, int, float, double, double> tc{ };
    
    auto q = tc.count_if<0, tc.size(), lam0>();
    // q == 2
    auto w = tc.remove<0,1,2,3>();
    // w == type_container<double, double>
    auto e = tc.remove<>();
    // e == type_container<char, short, int, float, double, double>
    auto r = tc.remove<decltype(tc.at<0>()), decltype(tc.at<1>()), decltype(tc.at<2>())>();
    // r == type_container<float, double, double>
    auto t = tc.at<3>();
    // t == type_pair<3, float>

    constexpr auto lam1 = [] <typename T> () { std::print("hello, world\n"); };
    tc.for_each<0, tc.size(), lam1>();
    // it print "hello, world\n" 6 times.

    return 0;
}
```