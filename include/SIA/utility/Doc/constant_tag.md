# Constant Tag
constant_tag takes multiple template entity enum class and give user functionality query if constant_tag has target enum class.  
this functionality may help save struct size, help compiler oprimization.
```cpp
struct A
{
    // this is common case of use tag as choose processing.
    unsigned int tag;
    void proc()
    {
        if (tag == 0) { /* proc other... */ }
        else if (tag == 1) { /* proc other... */ }
        else { /* proc other... */ }
    }
};

enum class ec { a, b };
template <auto... Es>
struct B : sia::constant_tag<Es...>
{
    // if we know tag is constant / deside at declaration.
    // use constant_tag could be good choice.
    using tag_t = sia::constant_tag<Es...>;
    void proc()
    {
        if (tag_t::query(ec::a)) { /* proc other... */ }
        else if (tag_t::query(ec::b)) { /* proc other... */ }
        else { /* proc other... */ }
    }
};

int main()
{
    A obj0 { };
    obj0.proc();

    B<ec::a> obj1 { };
    obj1.proc();
    
    return 0;
}
```