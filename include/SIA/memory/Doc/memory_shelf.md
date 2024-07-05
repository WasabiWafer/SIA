```cpp
sia::memory_shelf<4, 512> ms { };
//<PageNum, WordNum, WordType(default unsigned char), WorTypeSize(default sizeof(WordType))>

ms.assign(1); // assign vector
std::size_t* r0 = ms.allocate<std::size_t>(1, sia::memory_shelf_policy::policy::none); // allocate size_t 1

ms.deallocate(r0, 1); // deallocate

int* r1 = ms.allocate<int>();
bool cond = ((void*)r0 == (void*)r1); // it's true
// memory_shelf don't modify user's ptr. must care by user.
ms.deallocate(r1);

r1 = ms.allocate<int>(10); // allocate sizeof(int)*10
new (r1)(int[10]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; // inplacement new initialization fine.
cond = ((void*)r0 == (void*)r1); // it's false
// memory_shelf don't automatically reorganization memory use status.

ms.restore();
// make sequential memory information from use concatable deallocated memory.
// and delete deallocated information when it concat.
// restore() has 2 step, first call recycle() , second call recover() (third call update_page(update page info with iteration))
// recycle() - collecte concatable deallocated memory infomation and make concat infomation
// recover() - if its page infomation get reachable sequentially to the end, it generate sequential infoamtion and erase the deallocated information.

sia::memory_shelf_policy::policy::none;
// use default allocation process.
// try use deallocated memory -> try use sequential memory
sia::memory_shelf_policy::policy::poor;
// try allocate only use deallocated memory.
sia::memory_shelf_policy::policy::rich;
// try allocate only use sequential memory.
sia::memory_shelf_policy::policy::thrifty;
// try allocate only use deallocated memory but it call recycle() before try allocate and call recover() after (and update_page()).

// you can use specific book/page allocation.
allocate(size, policy) // common case
allocate(pos0, size, policy) // specific book allocation request
allocate(pos0, pos1, size, policy) // specific page allocation request

// also restore()
restore()
restore(pos0)
restore(pos0, pos1)

// get size function
word_size()
page_size()
book_size()
shelf_size()
capacity()

// get pos info from address
addr_pos(ptr)
// it return sia::tuple<bool, size_t, size_t, size_t> which represent {is_valid, pos0, pos1, pos2}
// pos2 is word pos
```