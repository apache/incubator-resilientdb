#ifndef _HASH_MAP_H_
#define _HASH_MAPS_H_

#include <iostream>
#include <unordered_map>
#include <mutex>

template <class _key, class _val>
class SpinLockMap
{
public:
    SpinLockMap(){};
    ~SpinLockMap() { this->map.clear(); }

    bool exists(_key key)
    {
        bool result = false;
        this->lock.lock();
        result = this->map.count(key);
        this->lock.unlock();
        return result;
    }
    bool check_and_get(_key key, _val &val)
    {
        bool result = false;
        this->lock.lock();
        result = this->map.count(key);
        if (result)
            val = this->map[key];
        this->lock.unlock();
        return result;
    }
    _val get(_key key)
    {
        this->lock.lock();

        assert(this->map.count(key));
        _val temp = this->map[key];

        this->lock.unlock();
        return temp;
    };

    void add(_key key, _val value)
    {
        this->lock.lock();
        this->map[key] = value;
        this->lock.unlock();
    };
    int remove(_key key)
    {
        
        this->lock.lock();

        int result = this->map.erase(key);

        this->lock.unlock();
        return result;
    }

    _val pop(_key key)
    {
        _val temp;
        
        this->lock.lock();
        if (this->map.count(key))
        {
            temp = this->map[key];
            this->map.erase(key);
        }
        else
        {
            temp = 0;
        }
        this->lock.unlock();
        return temp;
    }
    int size()
    {
        return map.size();
    }

private:
    std::unordered_map<_key, _val> map;
    std::mutex lock;
};

#endif