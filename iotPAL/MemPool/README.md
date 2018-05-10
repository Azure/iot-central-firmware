### MemPool - Memory Pool

Memory management library for fragmenting less and having more control over allocations.

Usage:
```
iotPAL::MemPool *memory = iotPAL::MemPool::New(4096);

void * mem = memory->alloc(211); // filled with zero

memory->free(mem);
```

- `static iotPAL::MemPool::New(size)`
Create a new instance of MemPool

- `alloc(size)`
Allocate `size` amount of bytes. Returns `NULL` if not available.
Minimum chunk size is 32 bytes. So, even if you call `alloc(20)`
you will consume 32 bytes.

- `free(addr)`
Free the allocated address `addr`

- `getTotalFree()`
Returns total free bytes in the pool.
!!  be careful while interpreting it.
i.e. total free 320 bytes might mean 10 individual 32 bytes fragmented.
Thus, there is no guarantee that you can allocate the given amount.

#### TEST

- Visit `test/` folder
- `make && ./app.o > new_baseline && diff baseline new_baseline`

If the output you see is limited to `make` output below (or similar), your changes are good!

```
g++ -c ../mempool.cpp -DVERBOSE_LOGGING_ENABLED=1 -o mempool.o
g++ -c test.cpp -o test.o
g++ -o app.o test.o mempool.o
rm -rf test.o mempool.o
```

#### MemPool ChangeLog

##### v0.0.1

- initial impementation
- alloc / free / getTotalFree interfaces.
- Default UNIT block size is 32 bytes
- implement unit tests

