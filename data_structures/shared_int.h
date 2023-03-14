#ifndef _HASH_MAP_H_
#define _HASH_MAPS_H_

#include <iostream>
#include <unordered_map>
#include <mutex>

class SharedInt
{
public:
    SharedInt(){};
    ~SharedInt() {}

    uint64_t get()
    {
        uint64_t result;
        this->lock.lock();
        result = this->value;
        this->lock.unlock();
        return result;
    };
    void set(uint64_t val)
    {
        this->lock.lock();
        this->value = val;
        this->lock.unlock();
    };

    void add(uint64_t val)
    {
        this->lock.lock();
        this->value += val;
        this->lock.unlock();
    };
    void sub(uint64_t val)
    {
        this->lock.lock();
        this->value -= val;
        this->lock.unlock();
    }

private:
    uint64_t value;
    std::mutex lock;
};

#endif