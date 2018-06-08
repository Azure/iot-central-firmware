#include <stdio.h>
#include <assert.h>
#include "../mempool.h"

#define MEMORY_UNIT_SIZE 32

int main()
{
    // Test alloc / free all blocks
    {
        iotPAL::MemPool *memory = iotPAL::MemPool::New(4096);
        void *addrs[128];

        for (int i = 0; i < 128; i++) {
            addrs[i] = memory->alloc(MEMORY_UNIT_SIZE);
            assert(addrs[i] != NULL);
            assert(memory->getTotalFree() == 4096 - ((i + 1) * MEMORY_UNIT_SIZE));
        }

        for (int i = 0; i < 128; i++) {
            memory->free(addrs[i]);
            assert(memory->getTotalFree() == ((i + 1) * MEMORY_UNIT_SIZE));
        }

        delete memory;
    }

    // Test alloc / free all blocks 121 // force 2 blocks
    {
        iotPAL::MemPool *memory = iotPAL::MemPool::New(4096);
        void *addrs[32];

        for (int i = 0; i < 32; i++) {
            addrs[i] = memory->alloc(121);
            assert(addrs[i] != NULL);
            assert(memory->getTotalFree() == 4096 - ((i + 1) * 128));
        }

        for (int i = 0; i < 32; i++) {
            memory->free(addrs[i]);
            assert(memory->getTotalFree() == ((i + 1) * 128));
        }

        delete memory;
    }

    // Test alloc fragment
    {
        iotPAL::MemPool *memory = iotPAL::MemPool::New(4096);
        void * addr1 = memory->alloc(1024);
        assert(addr1 != NULL && memory->getTotalFree() == 3072);

        void * addr2 = memory->alloc(2048);
        assert(addr2 != NULL && memory->getTotalFree() == 1024);

        memory->free(addr1);
        assert(memory->getTotalFree() == 2048);

        addr1 = memory->alloc(2048); // should fail
        assert(addr1 == NULL);

        memory->free(addr2);
        assert(memory->getTotalFree() == 4096);

        addr2 = memory->alloc(2048);
        assert(addr2 != NULL && memory->getTotalFree() == 2048);

        delete memory;
    }

    return 0;
}