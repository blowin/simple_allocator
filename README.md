# BAlloc

### Simple example

```c++
#include "balloc.h"

#include <iostream>

using std::cout;
using std::endl;

struct test
{
    int x;
    test(int _x) : x(_x) { cout << "create test x: " << x << "\n";}
    ~test() { cout << "destroy test x: " << x << "\n";}
};

int main()
{
    test* t1 = static_cast<test*>(block_allocator<test>::operator new(sizeof(test))); // get allocate memoy from pool
    new(t1)test(20); // create test x: 20
    
    t1->~test(); // "destroy test x: 20
    block_allocator<test>::operator delete(t1, sizeof(test)); // return to pool
    
    //or
    
    // alloc memory and call constructor
    auto t2 = block_allocator<test>::allocate(10); // create test x: 10
    // call dctor and return memory to pool
    block_allocator<test>::dealloc(t2); // destroy test x: 10
    return 0;
}
```
