#ifndef _MEM_ALLOC_H_
#define _MEM_ALLOC_H_

#include "global.h"

class mem_alloc
{
public:
    void *alloc(uint64_t size);
    void *align_alloc(uint64_t size);
    void *realloc(void *ptr, uint64_t size);
    void free(void *block, uint64_t size);
};

#endif
