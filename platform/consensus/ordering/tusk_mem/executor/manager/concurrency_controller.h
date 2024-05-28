#pragma once

#include "service/contract/executor/manager/data_storage.h"

#include <map>
#include <shared_mutex>

namespace resdb {
namespace contract {

enum State{
    LOAD = 0,
    STORE = 1,
    REMOVE = 2,
};

struct Data{
  State state;
  uint256_t data;
  int64_t version;
  uint256_t old_data;
  Data(){}
  Data(const State& state):state(state){}
  Data(const State& state, const uint256_t& data, int64_t version = 0)
    :state(state), data(data), version(version){}
  Data(const State& state, const uint256_t& data, int64_t version, const uint256_t& old_data)
    :state(state), data(data), version(version), old_data(old_data){}
    bool operator != (const Data& d) const{
      return d.state != this->state || d.data != this->data || d.version != this->version;
    }
};

class ConcurrencyController {
  public:
    ConcurrencyController(DataStorage * storage);

    typedef std::map<uint256_t, std::vector<Data>> ModifyMap;

    virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_) = 0;

    const DataStorage * GetStorage() const;

  protected:
    DataStorage * storage_;
};

}
} // namespace resdb
