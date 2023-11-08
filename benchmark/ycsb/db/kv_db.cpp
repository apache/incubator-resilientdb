#include "//benchmark/ycsb/db/kv_db.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace benchmark {

KVDB::KVDB(int batch_size){
  is_stop_ = false;
  idx_ = 1;
  finish_ = false;
  user_id_ = 0;
  batch_size_ = batch_size;
    process_thread_ = std::thread([&]() {
        while(!is_stop_){
          if(Process()){
            continue;
          }
          if(finish_){
            break;
          }
        }
    });
}

KVDB::~KVDB() {
  is_stop_ = true;
  if(process_thread_.joinable()){
    process_thread_.join();
  }
}

void KVDB::Wait(){
  finish_ = true;
  if(process_thread_.joinable()){
    process_thread_.join();
  }
}

std::string KVDB::ConvertToInt(const std::string& value) {
  std::lock_guard<std::mutex> lk(mutex_);
  if(data_.find(value) == data_.end()){
    data_[value] = idx_++;
  }
  return std::to_string(data_[value]);
}

int KVDB::Read(const std::string &table, const std::string &key,
         const std::vector<std::string> *fields,
         std::vector<KVPair> &result) {
  //LOG(ERROR)<<"get field:"<<key;
  Params func_params;
  func_params.set_func_name("get(uint256)");
  func_params.add_param(ConvertToInt(key));

  ContractExecuteInfo info(GetAccountAddress(), GetContractAddress(), "", func_params, 0);
  request_.Push(std::make_unique<QueryItem>(GetCurrentTime(), info));
  return ycsbc::DB::kOK;
}

int KVDB::Update(const std::string &table, const std::string &key,
    std::vector<KVPair> &values) {
  Params func_params;
  func_params.set_func_name("set(uint256,uint256)");
  func_params.add_param(ConvertToInt(key));
  func_params.add_param(ConvertToInt(values[0].second));
  //LOG(ERROR)<<"update key:"<<key;

  ContractExecuteInfo info(GetAccountAddress(), GetContractAddress(), "", func_params, 0);

  request_.Push(std::make_unique<QueryItem>(GetCurrentTime(), info));
  return ycsbc::DB::kOK;
}

Address KVDB::GetAccountAddress() {
  return contract_executor_->GetAccountAddress();
}

Address KVDB::GetContractAddress() {
  return contract_executor_->GetContractAddress();
}


void KVDB::AddLatency(uint64_t latency){
  latency_list_.push_back(latency);
}

void KVDB::AddRetryTime(int retry_time) {
  retry_time_.push_back(retry_time);
}

std::vector<uint64_t> KVDB::GetLatencys() {
  return latency_list_;
}

std::vector<int> KVDB::GetRetryTime() {
  return retry_time_;
}

bool KVDB::Process (){
      int batch_size = batch_size_;
      auto item = request_.Pop();
      if(item == nullptr) {
        return false;
      }
      std::vector<ContractExecuteInfo> info_list;
      item->info.user_id = user_id_++;
      times_[item->info.user_id] = item->start_time;

      info_list.push_back(item->info);
      while(!is_stop_ && info_list.size() < static_cast<size_t>(batch_size)){
        auto item = request_.Pop(0);
        if(item == nullptr){
          break;
        }

        item->info.user_id = user_id_++;
        times_[item->info.user_id] = item->start_time;
        info_list.push_back(item->info);
      }

      if(info_list.size()>0){
        std::vector<std::unique_ptr<ExecuteResp>> ret = contract_executor_->Execute(info_list);
        uint64_t end_time = GetCurrentTime();
        for(auto& resp: ret){
          int user_id = resp->user_id;
          int latency = end_time - times_[user_id];
          AddLatency(latency);
          AddRetryTime(resp->retry_time);
        }
        times_.clear();
      }
      return true;
    }


} // namespace ycsbc
}