#ifndef _HASH_MAP_H_
#define _HASH_MAPS_H_

#include <iostream>
#include <set>
#include <mutex>

template <class _type>
class SpinLockSet
{
public:
    SpinLockSet(){};
    ~SpinLockSet() { this->set.clear(); }

    int exists(_type key)
    {
        bool result = false;
        this->lock.lock();
        result = this->set.count(key);
        this->lock.unlock();
        return result;
    }
    void add(_type key)
    {
        this->lock.lock();
        this->set.insert(key);
        this->lock.unlock();
    };
    int check_and_add(_type key)
    {
        bool result = false;
        this->lock.lock();
        result = this->set.count(key);
        this->set.insert(key);
        this->lock.unlock();
        return result;
    }
    int remove(_type key)
    {
        this->lock.lock();

        int result = this->set.erase(key);

        this->lock.unlock();
        return result;
    }

    int size()
    {
        return set.size();
    }

private:
    std::set<_type> set;
    std::mutex lock;
};

#endif