# Tuple
the Tuple is almost same as std::tuple.  
but its use different design decision.  
MSVC is implement tuple with recursive struct, but sia::tuple is not.
sia::tuple offer 'at' functionality which std::tuple doesn't. (but std::get not offer)

```cpp
int main()
{
    sia::tuple tp0{'0', 1, 2.f, (double)3};
    // sia::tuple<char, int, float, double>
    tp0.at<0>() = 'T'; // it return reference
    // -> tp0 ['T', 1, 2.0, 3.0]
    return 0;
}
```