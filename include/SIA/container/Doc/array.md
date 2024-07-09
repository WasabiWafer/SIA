# Array
 sia::array is mostly same std::array but it offer initialize deduction guide.  
 it automatically make fittable size array.  
 Some functions offered by std::array are not offer. (like rbegin, cbeing...)
```cpp
int main()
{
    // sia::array constexpr-able
    static constexpr sia::array arr0 {"hello, sia::array!"};
    // initialize arr0 ['h' 'e' 'l' 'l' 'o' ',' ' ' 's' 'i' 'a' ':' ':' 'a' 'r' 'r' 'a' 'y' '!']
    
    std::array arr1 { "hello, std::array!"};
    // initialize arr1 [ "hello, std::array!" ]
    
    for (auto& elem : arr0) { std::print("{}", elem); }
    // it support begin, end

    sia::array arr1{arr0, arr0, arr0};
    // sia::array<const sia::array<const char, 19>, 3>

    constexpr std::string_view sv {arr0.begin(), arr0.size()}; // awsome! static storage string_view!
    // if sia::array is static and char array it can be string_view easily.
    return 0;
}
```