# Array
 sia::array is mostly same std::array but it offer initialize deduction guide.  
 it automatically make fittable size array.  
 Some functions offered by std::array are not offer. (like rbegin, cbeing...)
```cpp
int main()
{
    constexpr sia::array arr0 {"hello, sia::array!"};
    // initialize arr0 ['h' 'e' 'l' 'l' 'o' ',' ' ' 's' 'i' 'a' ':' ':' 'a' 'r' 'r' 'a' 'y' '!']
    std::array test0{1,2,3,4,5};
    // array<const int, 5>
    sia::array test1{test0, test0};
    // array<array<const int, 5>, 2>
    sia::array test2{test1, test1};
    // array<array<array<const int, 5>, 2>, 2>
    static constexpr sia::array test3{"hello, world !\n", "hello, world !\n", "hello, world !\n", "hello, world !\n", "hello, world !\n"};
    // sia::array<sia::array<const char, 16>, 5>
    constexpr std::string_view sv {test3.begin(), test3.size()};
    // sv == "hello, world !\n\0hello, world !\n\0hello, world !\n\0hello, world !\n\0hello, world !\n\0"
    std::print("{}", sv);
    // print "hello, world !"" 5 lines
    return 0;
}
```