#include "common/dispatcher.h"
#include "common/trace.h"
#include <openenclave/advanced/mallinfo.h>


ecall_dispatcher::ecall_dispatcher() : counterNum(0)
{
}

int ecall_dispatcher::get_heap_stats(
    uint64_t* current_heap,
    uint64_t* peak_heap,
    uint64_t* max_heap)
{
    if (!current_heap || !peak_heap || !max_heap)
        return 1;

    oe_mallinfo_t info = {};
    oe_result_t r = oe_allocator_mallinfo(&info);
    if (r != OE_OK)
        return 1;

    *current_heap = (uint64_t)info.current_allocated_heap_size;
    *peak_heap    = (uint64_t)info.peak_allocated_heap_size;
    *max_heap     = (uint64_t)info.max_total_heap_size;

    return 0;
}