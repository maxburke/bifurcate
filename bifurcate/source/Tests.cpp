#include "Component.h"

#ifndef NDEBUG
static void TestComponentPool()
{
    bc::FixedBlockAllocator pool;
    const size_t numAllocations = 16;
    pool.Initialize(numAllocations, 16, 12);

    void *ptrs[numAllocations] = {};
    for (size_t i = 0; i < numAllocations; ++i)
    {
        void *p = pool.Allocate();
        assert(((uintptr_t)p & 15) == 0);
        assert(p != NULL);
        assert(pool.GetObjectAtIndex<void>(i) == p);

        ptrs[i] = p;
    }

    void *shouldBeNull = pool.Allocate();
    assert(shouldBeNull == NULL);

    void *p = ptrs[11];
    pool.Free(p);
    ptrs[11] = pool.Allocate();
    assert(p == ptrs[11]);

    for (size_t i = 0; i < numAllocations; ++i)
    {
        pool.Free(ptrs[i]);
        ptrs[i] = NULL;
    }

    for (size_t i = 0; i < numAllocations; ++i)
    {
        ptrs[i] = pool.Allocate();
        assert(ptrs[i] != NULL);
    }

    for (size_t i = 0; i < numAllocations; ++i)
        pool.Free(ptrs[i]);

    pool.Teardown();
}

void RunTests()
{
    TestComponentPool();
}

#else

void RunTests()
{
}

#endif
