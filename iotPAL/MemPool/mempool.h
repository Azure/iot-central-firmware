// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef IOT_PAL_MEMPOOL_H
#define IOT_PAL_MEMPOOL_H

#include <stddef.h> // size_t etc.

#define IOT_PAL_MEMPOOL_VERSION "0.0.1"

namespace iotPAL
{

class MemPool
{
    unsigned char * blocks;
    char *          space;
    size_t          total;
    unsigned        available; // available number of 32 byte blocks

    // returns the index of next available block
    // returns -1 if non available
    long indexOfNextAvailable(unsigned startIndex, unsigned count);

    // alloc given `numberOfBlocks` starting from `startIndex`
    // !! does not implement failfast but has asserts
    void * allocUnchecked(unsigned numberOfBlocks);

    MemPool(size_t size);

public:
    ~MemPool();

    static MemPool * New(size_t size);

    void * alloc(size_t size);
    void free(void *addressToFree);

    // accurate but be careful while interpreting it.
    // i.e. total free 40 bytes might mean 5 individual 8 bytes fragmented.
    // Thus, there is no guarantee that you can allocate the given amount.
    size_t getTotalFree();
}; // MemPool

} // iotPAL

#endif // IOT_PAL_MEMPOOL_H