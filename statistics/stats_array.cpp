#include "global.h"
#include "stats.h"
#include "mem_alloc.h"
#include "client_txn.h"
#include "work_queue.h"
#include <time.h>
#include <sys/times.h>
#include <sys/vtimes.h>

void StatsArr::quicksort(int low_idx, int high_idx)
{
    int low = low_idx;
    int high = high_idx;
    uint64_t pivot = arr[(low + high) / 2];
    uint64_t tmp;

    while (low < high)
    {
        while (arr[low] < pivot)
            low++;
        while (arr[high] > pivot)
            high--;
        if (low <= high)
        {
            tmp = arr[low];
            arr[low] = arr[high];
            arr[high] = tmp;
            low++;
            high--;
        }
    }

    if (low_idx < high)
        quicksort(low_idx, high);
    if (low < high_idx)
        quicksort(low, high_idx);
}

void StatsArr::init(uint64_t size, StatsArrType type)
{
    arr = (uint64_t *)
              mem_allocator.alloc(sizeof(uint64_t) * (size + 1));
    for (uint64_t i = 0; i < size + 1; i++)
    {
        arr[i] = 0;
    }
    this->size = size + 1;
    this->type = type;
    cnt = 0;
    pthread_mutex_init(&mtx, NULL);
}

void StatsArr::free()
{
    mem_allocator.free(arr, 0);
}

void StatsArr::clear()
{
    cnt = 0;
}

void StatsArr::resize()
{
    size = size * 2;
    arr = (uint64_t *)
              mem_allocator.realloc(arr, sizeof(uint64_t) * size);
    for (uint64_t i = size / 2; i < size; i++)
    {
        arr[i] = 0;
    }
}

void StatsArr::insert(uint64_t item)
{
    pthread_mutex_lock(&mtx);
    if (type == ArrIncr)
    {
        uint64_t my_cnt = ATOM_FETCH_ADD(cnt, 1);
        if (my_cnt == size)
            resize();
        arr[my_cnt] = item;
    }
    else if (type == ArrInsert)
    {
        if (item >= size)
        {
            ATOM_ADD(arr[size - 1], 1);
        }
        else
        {
            ATOM_ADD(arr[item], 1);
        }
        ATOM_ADD(cnt, 1);
    }
    pthread_mutex_unlock(&mtx);
}

void StatsArr::append(StatsArr array)
{
    for (uint64_t i = 0; i < array.cnt; i++)
    {
        insert(array.get_idx(i));
    }
}

void StatsArr::print(FILE *f)
{
    if (type == ArrIncr)
    {
        for (UInt32 i = 0; i < cnt; i++)
        {
            fprintf(f, "%ld,", arr[i]);
        }
    }
    else if (type == ArrInsert)
    {
        for (UInt64 i = 0; i < size; i++)
        {
            if (arr[i] > 0)
                fprintf(f, "%ld=%ld,", i, arr[i]);
        }
    }
}

void StatsArr::print(FILE *f, uint64_t min, uint64_t max)
{
    if (type == ArrIncr)
    {
        for (UInt32 i = min * cnt / 100; i < max * cnt / 100; i++)
        {
            fprintf(f, "%ld,", arr[i]);
        }
    }
}

uint64_t StatsArr::get_idx(uint64_t idx)
{
    return arr[idx];
}

uint64_t StatsArr::get_percentile(uint64_t ile)
{
    return arr[ile * cnt / 100];
}

uint64_t StatsArr::get_avg()
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < cnt; i++)
    {
        sum += arr[i];
    }
    if (cnt > 0)
        return sum / cnt;
    return 0;
}
