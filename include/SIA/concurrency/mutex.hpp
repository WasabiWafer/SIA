#pragma once

namespace sia
{
    // need atomic for consistency
    struct mutex { bool flag{false}; };
    bool test_and_set(mutex& mtx)
    {
        bool old_val = mtx.flag;
        mtx.flag = true;
        return old_val;
    }
    void lock(mutex& mtx) { while(test_and_set(mtx)){ } }
    void unlock(mutex& mtx) { mtx.flag = false; }
} // namespace sia
