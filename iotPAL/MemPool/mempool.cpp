// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "mempool.h"
#include <assert.h>
#include <string.h>

#ifndef VERBOSE_LOGGING_ENABLED
#define LOG_VERBOSE(...)
#else
#include <stdio.h>
#define LOG_VERBOSE(...) \
    do { \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while(0)
#endif

namespace iotPAL
{

#define IS_BIT_SET(ch, ind) (!!((ch) & (1 << (ind))))
#define SET_BIT(ch, ind, to) (ch) ^= (-(to) ^ (ch)) & (1UL << (ind))

#define MEMORY_UNIT_SIZE 32

// STATUS BIT SCHEMA
// FIRST BIT:  1 - allocated - 0 free
// SECOND BIT: (matters only if FIRST BIT is set)
//             1 - first block of following blocks - 0 following block

// 1 byte == 8 bits / 2 (two bits schema)
const unsigned BIT_SCHEMA_PER_ONE_BLOCK = (sizeof(char) * 8) / 2;

inline void READ_BIT(unsigned char bt, int index, bool &isAllocated, bool &isFirstBlock) {
    assert(index < 4); // index * 2 < 8
    isAllocated = IS_BIT_SET(bt, index * 2);
    isFirstBlock = !isAllocated ? false : (IS_BIT_SET(bt, (index * 2) + 1));
}

inline void WRITE_BIT(unsigned char *bt, int index, bool isAllocated, bool isFirstBlock) {
    assert(index < 4); // index * 2 < 8
    SET_BIT(*bt, index * 2, isAllocated);
    SET_BIT(*bt, (index * 2) + 1, isAllocated && isFirstBlock);
}

MemPool::~MemPool()
{
    if (blocks != NULL)
    {
        ::free(blocks);
        blocks = NULL;
        space = NULL;
        available = 0;
        LOG_VERBOSE("- MemPool::~MemPool. Free %lu bytes", total);
    }
}

MemPool::MemPool(size_t size): blocks(NULL), space(NULL), total(size), available(0)
{
    // size % 256 == 0
    assert(total % (MEMORY_UNIT_SIZE * BIT_SCHEMA_PER_ONE_BLOCK) == 0);
    const unsigned
    NUMBER_OF_BLOCKS = total / MEMORY_UNIT_SIZE;

    const unsigned
    BLOCKS_ALLOC_SIZE = NUMBER_OF_BLOCKS / BIT_SCHEMA_PER_ONE_BLOCK;

    unsigned total_memory_needed =
          BLOCKS_ALLOC_SIZE /* see STATUS BIT SCHEMA */
        + total;

    char * mem = (char*) malloc (total_memory_needed);
    assert(mem != NULL);
    memset(mem, 0, total_memory_needed);

    blocks = (unsigned char *) mem;
    space = (char*) (mem + BLOCKS_ALLOC_SIZE);

    available = NUMBER_OF_BLOCKS;
}

long MemPool::indexOfNextAvailable(unsigned startIndex, unsigned count)
{
    LOG_VERBOSE("- MemPool::indexOfNextAvailable startIndex:(%u) count:(%u)"
                " available:(%u)",
                startIndex, count, available);
    if (count > available)
    {
        return -1;
    }

    const unsigned
    NUMBER_OF_BLOCKS = total / MEMORY_UNIT_SIZE;

    const unsigned
    BLOCKS_ALLOC_SIZE = NUMBER_OF_BLOCKS / BIT_SCHEMA_PER_ONE_BLOCK;

    long startBlock = -1;
    unsigned endBlock = -1;
    while (startIndex + count <= NUMBER_OF_BLOCKS)
    {
        const unsigned bytePos = startIndex / BIT_SCHEMA_PER_ONE_BLOCK;
        const unsigned bitPos = startIndex % BIT_SCHEMA_PER_ONE_BLOCK;
        unsigned char ch = *(blocks + bytePos);

        bool isAllocated = false, isFirstBlock = false;
        READ_BIT(ch, bitPos, isAllocated, isFirstBlock);
        LOG_VERBOSE("- - reading bit @ byte:(%u) bit:(%u) "
                    "isAllocated:(%d) isFirstBlock:(%d)", bytePos, bitPos,
                    isAllocated, isFirstBlock);
        if (startBlock == -1) {
            // find the starting block
            if (!isAllocated)
            {
                startBlock = startIndex;
                endBlock = startIndex + count - 1;
            }
        } else {
            // see if have `count` amount of blocks free
            if (isAllocated)
            {
                startBlock = -1;
                endBlock = -1;
            }
        }

        if (endBlock == startIndex)
        {
            return startBlock;
        }

        startIndex++;
    }

    return startBlock; // no available
}

void * MemPool::allocUnchecked(unsigned numberOfBlocks)
{
    const unsigned
    NUMBER_OF_BLOCKS = total / MEMORY_UNIT_SIZE;

    const unsigned
    BLOCKS_ALLOC_SIZE = NUMBER_OF_BLOCKS / BIT_SCHEMA_PER_ONE_BLOCK;

    assert (numberOfBlocks < NUMBER_OF_BLOCKS);

    long index = indexOfNextAvailable(0, numberOfBlocks);
    if (index == -1)
    {
        LOG_VERBOSE("- MemPool::allocUnchecked total (%u) NOT AVAILABLE",
                numberOfBlocks * MEMORY_UNIT_SIZE);
        return NULL;
    }

    for (long i = index, j = index + numberOfBlocks; i < j; i++)
    {
        const unsigned bytePos = i / BIT_SCHEMA_PER_ONE_BLOCK;
        const unsigned bitPos = i % BIT_SCHEMA_PER_ONE_BLOCK;
        unsigned char * ch = (blocks + bytePos);

        WRITE_BIT(ch, bitPos, true, i == index);
        LOG_VERBOSE("- - - writing bit @ byte:(%u) bit:(%u) "
                    "isAllocated:(%d) isFirstBlock:(%d)", bytePos, bitPos,
                    true, i == index);
    }

    available -= numberOfBlocks;

    char * addr = space + (index * MEMORY_UNIT_SIZE);
    LOG_VERBOSE("- MemPool::allocUnchecked total (%u bytes / %u blocks) @ index(%ld)",
                numberOfBlocks * MEMORY_UNIT_SIZE, numberOfBlocks, index);

    return addr;
}

void MemPool::free(void *startAddress)
{
    LOG_VERBOSE("- MemPool::free");

    const unsigned
    NUMBER_OF_BLOCKS = total / MEMORY_UNIT_SIZE;

    const unsigned
    BLOCKS_ALLOC_SIZE = NUMBER_OF_BLOCKS / BIT_SCHEMA_PER_ONE_BLOCK;

    assert ((char*)startAddress >= space);

    size_t startIndex = ((size_t)startAddress - (size_t)space) / MEMORY_UNIT_SIZE;
    assert (startIndex < NUMBER_OF_BLOCKS);

    long i;
    for (i = startIndex; i < NUMBER_OF_BLOCKS; i++)
    {
        const unsigned bytePos = i / BIT_SCHEMA_PER_ONE_BLOCK;
        const unsigned bitPos = i % BIT_SCHEMA_PER_ONE_BLOCK;
        unsigned char * ch = (blocks + bytePos);

        bool isAllocated = false, isFirstBlock = false;
        READ_BIT(*ch, bitPos, isAllocated, isFirstBlock);

        if ( isAllocated &&
             ( (i == startIndex && isFirstBlock) ||
               (i != startIndex && !isFirstBlock) ) )
        {
            WRITE_BIT(ch, bitPos, false, false);
            LOG_VERBOSE("- - - writing bit @ byte:(%u) bit:(%u) "
                    "isAllocated:(%d) isFirstBlock:(%d)", bytePos, bitPos,
                    false, false);
        } else {
            break;
        }
    }

    available += (i - startIndex);
    LOG_VERBOSE("- MemPool::free total (%lu)", (i - startIndex) * MEMORY_UNIT_SIZE);
}

MemPool * MemPool::New(size_t size)
{
    MemPool * mempool = new MemPool(size);
    LOG_VERBOSE("- MemPool::New size(%lu)", size);

    return mempool;
}

void * MemPool::alloc(size_t size)
{
    size_t blockCount = (size / MEMORY_UNIT_SIZE) +
                        ((size % MEMORY_UNIT_SIZE != 0) ? 1 : 0);

    LOG_VERBOSE("- MemPool::alloc %lu bytes (%lu blocks)", size, blockCount);
    return allocUnchecked(blockCount);
}

size_t MemPool::getTotalFree()
{
    return available * MEMORY_UNIT_SIZE;
}

} // iotPAL