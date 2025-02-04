/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "service/contract/tusk_service/contract_transaction_manager.h"

#include <glog/logging.h>

#include "service/contract/tusk_service/execution_helper.h"

namespace resdb {
namespace contract {

using resdb::contract::x_manager::AddressManager;
using resdb::contract::x_manager::ContractManager;
using resdb::contract::x_manager::StreamingCommitter;

bool use_single = false;

ContractTransactionManager::ContractTransactionManager(Storage* storage) {
  if (use_single) {
    manager_ = std::make_unique<ContractManager>(
        std::make_unique<DataStorage>(), 1, ContractManager::Options::SEQ);
  } else {
    manager_ = std::make_unique<ContractManager>(
        std::make_unique<DataStorage>(), 16, ContractManager::Options::FX);
  }
  LOG(ERROR) << " user single:" << use_single;
  address_manager_ = std::make_unique<AddressManager>();
  dg_committer_ = std::make_unique<StreamingCommitter>(
      manager_->GetStorage(), manager_->GetState(),
      [&](int seq) {
        std::unique_ptr<resdb::Request> req;
        std::unique_ptr<resdb::BatchUserRequest> batch_req;
        {
          std::unique_lock<std::mutex> lk(req_mutex_);
          req = std::move(req_seq_[seq]);
          assert(req != nullptr);
          batch_req = std::move(batch_req_[seq]);
          assert(batch_req != nullptr);
        }

        call_back_(*batch_req, std::move(req), nullptr);
      },
      16);
}

std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
ContractTransactionManager::Prepare(const BatchUserRequest& batch_request) {
  std::map<int, std::pair<std::unique_ptr<ContractExecuteInfo>,
                          std::unique_ptr<ModifyMap>>>
      ranklist;
  int seq = batch_request.seq();
  bool need_exe = false;
  // LOG(ERROR)<<"prepare seq:"<<seq;

  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    if (!request.ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      return nullptr;
    }
    if (request.cmd() == Request::CREATE_ACCOUNT) {
      std::unique_lock<std::mutex> lk(mutex_);
      if (!ExecutionHelper::CreateAccount(request.resp().account().address(),
                                          address_manager_.get())) {
        LOG(ERROR) << "create account fail";
        return nullptr;
      }
      LOG(ERROR) << "add account:" << request.resp().account().address();
    } else if (request.cmd() == Request::DEPLOY) {
      std::unique_lock<std::mutex> lk(mutex_);
      if (!ExecutionHelper::DeployWithAddress(request, address_manager_.get(),
                                              manager_.get())) {
        LOG(ERROR) << "deploy account fail";
        return nullptr;
      }
    } else if (request.cmd() == Request::EXECUTE) {
      if (request.func_params().is_only()) {
        continue;
      }
      ResultInfo info;
      assert(info.ParseFromString(sub_request.request().ext_data()));

      std::unique_ptr<ContractExecuteInfo> contract_info = nullptr;
      {
        std::unique_lock<std::mutex> lk(mutex_);
        contract_info =
            ExecutionHelper::GetContractInfo(request, address_manager_.get());
      }
      std::unique_ptr<ModifyMap> rws = ExecutionHelper::GetRWSList(info);
      if (rws->empty()) {
        need_exe = true;
      }
      ranklist[info.idx()] =
          std::make_pair(std::move(contract_info), std::move(rws));
      // LOG(ERROR)<<"get info idx:"<<info.idx()<<"
      // shard:"<<batch_request.belong_shards_size()<<" new exe:"<<need_exe<<"
      // shard id:"<<batch_request.shard_id();
    }
  }

  // LOG(ERROR)<<"prepare size:"<<ranklist.size()<<" seq:"<<seq;
  if (ranklist.empty()) {
    return nullptr;
  }

  if (need_exe) {
    ranklist.clear();
  }
  std::unique_lock<std::mutex> lk(mutex_);
  // LOG(ERROR)<<"prepare size:"<<ranklist.size()<<" seq:"<<seq<<"
  // size:"<<ranklist.empty();
  data_[seq] = std::move(ranklist);
  return nullptr;
}

std::unique_ptr<BatchUserResponse>
ContractTransactionManager::ExecutePreparedData(
    const BatchUserRequest& batch_request) {
  // LOG(ERROR)<<" execute parpare data";
  int seq = batch_request.seq();
  if (use_single) {
    ExecuteRequest(batch_request);
    return nullptr;
  } else {
    if (data_.find(seq) != data_.end()) {
      // LOG(ERROR)<<" contain seq:"<<data_[seq].size();
    }
    if (data_.find(seq) != data_.end() && data_[seq].empty()) {
      // LOG(ERROR)<<" execute parpare data with seq execu";
#define Async
#ifdef Async
      assert(call_back_ != nullptr);
      // call_back_(batch_request, std::move(request_), nullptr);
      AsyncExe(std::move(request_), batch_request);

      std::unique_ptr<BatchUserResponse> resp =
          std::make_unique<BatchUserResponse>();

      resp->set_is_async(true);
      return resp;
#else
      ExecuteRequest(batch_request);
#endif
    }
  }
  return nullptr;
  DataType rank_list;
  /*
  {
    std::unique_lock<std::mutex> lk(mutex_);
    auto it = data_.find(seq);
    if(it == data_.end()){
      return nullptr;
    }
    //manager_->VerifyContract(it->second);
    rank_list = std::move(it->second);
    data_.erase(it);
  }
  */
  std::vector<DataType> pt;
  // int size = data_.size();
  // LOG(ERROR)<<" contain verify size:"<<size;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    auto it = data_.find(seq);
    if (it == data_.end()) {
      return nullptr;
    }
    if (seq % 6 != 0) {
      return nullptr;
    }

    int ridx = 0;
    for (int i = 5; i >= 0; i--) {
      auto find_it = data_.find(seq - i);
      if (find_it == data_.end()) {
        continue;
      }
      // LOG(ERROR)<<"reset:"<<seq-i<<" size:"<<find_it->second.size();
      pt.push_back(std::move(find_it->second));
      data_.erase(find_it);
    }
  }
  // LOG(ERROR)<<" data block verify size:"<<data_.size();
  int ridx = 0;
  for (auto& item : pt) {
    for (auto& sit : item) {
      // LOG(ERROR)<<"reset seq:"<<seq<<" from :"<<(seq-i)<<"
      // idx:"<<sit.first<<" to:"<<ridx;
      rank_list[ridx++] = std::move(sit.second);
    }
  }

  // LOG(ERROR)<<"verify rank list:"<<rank_list.size();
  bool ret = manager_->VerifyContract(rank_list);
  // LOG(ERROR)<<" verify ret:"<<ret<<" seq:"<<seq<<" rank link
  // size:"<<rank_list.size();
  return nullptr;
}

std::unique_ptr<BatchUserResponse> ContractTransactionManager::ExecuteBatch(
    const BatchUserRequest& request) {
  if (!use_single) {
    VerifyAndExecuteRequest(request);
  } else {
    ExecuteRequest(request);
  }
  return nullptr;
}

bool ContractTransactionManager::ExecuteRequest(
    const BatchUserRequest& batch_request) {
  std::map<int, std::pair<std::unique_ptr<ContractExecuteInfo>,
                          std::unique_ptr<ModifyMap>>>
      ranklist;
  // LOG(ERROR)<<" execute:";

  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    if (!request.ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      return false;
    }
    if (request.cmd() == Request::CREATE_ACCOUNT) {
      absl::StatusOr<Account> account_or =
          ExecutionHelper::CreateAccountWithAddress(request.caller_address(),
                                                    address_manager_.get());
      if (!account_or.ok()) {
        LOG(ERROR) << "create account fail";
        return false;
      }
    } else if (request.cmd() == Request::DEPLOY) {
      absl::StatusOr<Contract> contract_or = ExecutionHelper::Deploy(
          request, address_manager_.get(), manager_.get());
      if (!contract_or.ok()) {
        LOG(ERROR) << "deploy account fail";
        return false;
      }
    } else if (request.cmd() == Request::EXECUTE) {
      std::unique_ptr<ContractExecuteInfo> contract_info =
          ExecutionHelper::GetContractInfo(request, address_manager_.get());
      auto ret = manager_->ExecContract(contract_info->caller_address,
                                        contract_info->contract_address,
                                        contract_info->func_params);
      // LOG(ERROR)<<" execute single contract:"<<ret.ok();
      assert(ret.ok());
    }
  }
  return true;
}

bool ContractTransactionManager::VerifyAndExecuteRequest(
    const BatchUserRequest& batch_request) {
  std::map<int, std::pair<std::unique_ptr<ContractExecuteInfo>,
                          std::unique_ptr<ModifyMap>>>
      ranklist;
  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    if (!request.ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      return false;
    }
    if (request.cmd() == Request::CREATE_ACCOUNT) {
      if (!ExecutionHelper::CreateAccount(request.resp().account().address(),
                                          address_manager_.get())) {
        LOG(ERROR) << "create account fail";
        return false;
      }
      LOG(ERROR) << "add account:" << request.resp().account().address();
    } else if (request.cmd() == Request::DEPLOY) {
      if (!ExecutionHelper::DeployWithAddress(request, address_manager_.get(),
                                              manager_.get())) {
        LOG(ERROR) << "deploy account fail";
        return false;
      }
    } else if (request.cmd() == Request::EXECUTE) {
      ResultInfo info;
      assert(info.ParseFromString(sub_request.request().ext_data()));

      std::unique_ptr<ContractExecuteInfo> contract_info =
          ExecutionHelper::GetContractInfo(request, address_manager_.get());
      std::unique_ptr<ModifyMap> rws = ExecutionHelper::GetRWSList(info);
      assert(rws->size() > 0);
      ranklist[info.idx()] =
          std::make_pair(std::move(contract_info), std::move(rws));
      // LOG(ERROR)<<"get info idx:"<<info.idx();
    }
  }

  if (ranklist.empty()) {
    LOG(ERROR) << "no ranklist.";
    return true;
  }

  // LOG(ERROR)<<"verify contract size:"<<ranklist.size();
  return manager_->VerifyContract(ranklist);
}

void ContractTransactionManager::AsyncExe(
    std::unique_ptr<resdb::Request> request,
    const BatchUserRequest& batch_request) {
  // LOG(ERROR)<<" execute parpare data, group id:"<<request->shard_id();
  int seq = batch_request.seq();
  int64_t group = 0;

  assert(request->shard_id() > 0);

  group |= 1ll << (request->shard_id() - 1);
  for (int g : request->belong_shards()) {
    // LOG(ERROR)<<" belong group:"<<g;
    group |= 1ll << (g - 1);
  }

  assert(group > 0);

  std::vector<std::pair<ContractExecuteInfo, int64_t>> contract_list;

  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    if (!request.ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      return;
    }
    std::unique_ptr<ContractExecuteInfo> contract_info =
        ExecutionHelper::GetContractInfo(request, address_manager_.get());

    manager_->GetFuncAddr(*contract_info);
    contract_info->commit_id = i + 1;
    contract_list.push_back(std::make_pair(*contract_info, group));
  }
  {
    std::unique_lock<std::mutex> lk(req_mutex_);
    req_seq_[seq] = std::move(request);
    batch_req_[seq] = std::make_unique<BatchUserRequest>(batch_request);
  }
  // LOG(ERROR)<<" async exe contract:"<<contract_list.size()<<"
  // group:"<<group<<"???";
  dg_committer_->AsyncExecContract(seq, contract_list);
}

}  // namespace contract
}  // namespace resdb
