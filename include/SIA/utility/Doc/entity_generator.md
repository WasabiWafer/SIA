# Entity Generator
entity_generator.hpp offer generate template entity similar monad way.  

```cpp
int main()
{
    constexpr auto lam0 = [] <int N> () { return N * 2; };
    constexpr auto lam1 = [] <int N> () { return N * 3; };
    
    sia::make_domain_sequence<std::make_integer_sequence<int, 5>, lam0> domain{ };
    // domain == entity_list<0, 2, 4, 8>
    sia::make_range_sequence<decltype(domain), lam0, lam1> range0{ };
    // range0 == entity_list<0, 4, 8, 16, 0, 6, 12, 24>
    sia::make_range_sequence<sia::entity_list<0,1,2,3,4,5>, lam1> range1{ };
    // range1 = entity_list<0, 2, 4, 6, 8, 10>

    sia::monadic<lam0, sia::entity_list<0,1,2,3,4,5>> mon0{ };
    auto mr0 = mon0.bind();
    // mr0 == monadic<lam0, entity_list<0,2,4,6,8,10>>
    auto mr1 = mr0.bind();
    // mr1 == monadic<lam0, entity_list<0,4,8,12,16,20>>
    auto el0 = mr1.result();
    // el0 == entity_list<0,4,8,12,16,20>

    return 0;
}
```