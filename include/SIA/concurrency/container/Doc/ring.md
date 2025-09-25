# Concurrency Ring Container
concurrency support ring container.

```cpp
// there is no blocking mechanism when using spsc container as mpmc container.
// should be careful.

#include "SIA/concurrency/container/ring.hpp"

using value_type = size_t;
constexpr size_t ring_size = 256;

sia::concurrency::ring<value_type, ring_size, sia::tags::producer::single, sia::tags::consumer::single> spsc_ring { };

sia::concurrency::ring<value_type, ring_size, sia::tags::producer::multiple, sia::tags::consumer::single> mpsc_ring { };

sia::concurrency::ring<value_type, ring_size, sia::tags::producer::single, sia::tags::consumer::multiple> spmc_ring { };

sia::concurrency::ring<value_type, ring_size, sia::tags::producer::multiple, sia::tags::consumer::multiple> mpmc_ring { };

spsc_ring.try_emplace_back(2);
value_type out {0};
spsc_ring.try_extract_front(out);
// out == 2;
```