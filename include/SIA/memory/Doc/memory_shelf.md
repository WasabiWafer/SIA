```cpp
sia::memory_shelf<4, 512> ms { };
//<PageNum, WordNum, WordType(default unsigned char), WorTypeSize(default 1)>

ms.assign(1);
// assign vector

std::size_t* r0 = ms.allocate<std::size_t>(1, sia::memory_shelf_policy::policy::none);
// allocate size_t

ms.deallocate(r0, 1);
// deallocate

int* r1 = ms.allocate<int>();
bool cond = ((void*)r0 == (void*)r1);
// it's true
// memory_shelf don't modify user's ptr. should care by user.
ms.deallocate(r1);

r1 = ms.allocate<int>(10);
// allocate sizeof(int)*10
new (r1)(int[10]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
// can be use inplacement new initialization.
cond = ((void*)r0 == (void*)r1);
// it's false
// memory_shelf don't automatically reorganization memory use status.

ms.restore();
// make sequential memory information from used concatable deallocated memory.
// and delete deallocated information when it concat.

sia::memory_shelf_policy::policy::none;
// use default allocation process.
// try use deallocated memory -> try use sequential memory -> return nullptr.
sia::memory_shelf_policy::policy::poor;
// try allocate only use deallocated memory.
sia::memory_shelf_policy::policy::rich;
// try allocate only use sequential memory.
sia::memory_shelf_policy::policy::thrifty;
// try allocate only use deallocated memory but it call restore() before try allocate.
```