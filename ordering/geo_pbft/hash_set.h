#include <iostream>
#include <mutex>
#include <set>

template <class _type>
class SpinLockSet {
 public:
  SpinLockSet(){};
  ~SpinLockSet() { set_.clear(); }

  int exists(_type key) {
    bool result = false;
    lock_.lock();
    result = set_.count(key);
    lock_.unlock();
    return result;
  }
  void add(_type key) {
    lock_.lock();
    set_.insert(key);
    lock_.unlock();
  };
  int check_and_add(_type key) {
    bool result = false;
    lock_.lock();
    result = set_.count(key);
    set_.insert(key);
    lock_.unlock();
    return result;
  }
  int remove(_type key) {
    lock_.lock();

    int result = set_.erase(key);

    lock_.unlock();
    return result;
  }

  int size() { return set_.size(); }

 private:
  std::set<_type> set_;
  std::mutex lock_;
};