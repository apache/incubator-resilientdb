
#include "mem_alloc.h"
#include "global.h"
#include "jemalloc/jemalloc.h"

//#define N_MALLOC

void mem_alloc::free(void *ptr, uint64_t size)
{
    if (NO_FREE)
    {
    }
    DEBUG_M("free %ld 0x%lx\n", size, (uint64_t)ptr);
#ifdef N_MALLOC
    std::free(ptr);
#else
    je_free(ptr);
#endif
}

void *mem_alloc::alloc(uint64_t size)
{
    void *ptr;

#ifdef N_MALLOC
    ptr = malloc(size);
#else
    ptr = je_malloc(size);
#endif
    DEBUG_M("alloc %ld 0x%lx\n", size, (uint64_t)ptr);
    assert(ptr != NULL);
    return ptr;
}

void *mem_alloc::align_alloc(uint64_t size)
{
    uint64_t aligned_size = size + CL_SIZE - (size % CL_SIZE);
    return alloc(aligned_size);
}

void *mem_alloc::realloc(void *ptr, uint64_t size)
{
#ifdef N_MALLOC
    void *_ptr = std::realloc(ptr, size);
#else
    void *_ptr = je_realloc(ptr, size);
#endif
    DEBUG_M("realloc %ld 0x%lx\n", size, (uint64_t)_ptr);
    return _ptr;
}
