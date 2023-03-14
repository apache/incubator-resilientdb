#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <mm_malloc.h>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include "helper.h"
#include "lock_free_queue.h"

using namespace std;

LockfreeQueue::LockfreeQueue()
{
    _head = NULL;
    _tail = NULL;
}

bool LockfreeQueue::enqueue(uintptr_t value)
{
    bool success = false;
    QueueEntry *new_head = (QueueEntry *)malloc(sizeof(QueueEntry));
    new_head->value = value;
    new_head->next = NULL;
    QueueEntry *old_head;
    while (!success)
    {
        old_head = _head;
        success = ATOM_CAS(_head, old_head, new_head);
        if (!success)
            PAUSE
    }
    if (old_head == NULL)
        _tail = new_head;
    else
        old_head->next = new_head;
    return true;
}

bool LockfreeQueue::dequeue(uintptr_t &value)
{
    bool success = false;
    QueueEntry *old_tail;
    QueueEntry *old_head;
    while (!success)
    {
        old_tail = _tail;
        old_head = _head;
        if (old_tail == NULL)
        {
            PAUSE
            return false;
        }
        if (old_tail->next != NULL)
            success = ATOM_CAS(_tail, old_tail, old_tail->next);
        else if (old_tail == old_head)
        {
            success = ATOM_CAS(_head, old_tail, NULL);
            if (success)
                ATOM_CAS(_tail, old_tail, NULL);
        }
        if (!success)
            PAUSE
    }
    value = old_tail->value;
    free(old_tail);
    return true;
}
