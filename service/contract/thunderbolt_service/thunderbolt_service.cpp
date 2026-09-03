/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "service/contract/thunderbolt_service/thunderbolt_service.h"

#include "common/utils/utils.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/leveldb_storage.h"
#include "service/contract/thunderbolt_service/execution_helper.h"

namespace resdb {
namespace contract {

using resdb::contract::x_manager::AddressManager;
using resdb::contract::x_manager::ContractManager;

const bool use_ov = false;

Thunderbolt::Thunderbolt(const ResDBConfig& config,
           std::unique_ptr<resdb::TransactionManager> executor)
    : thunderbolt::ThunderboltConsensus(config, std::move(executor)) {
  worker_num_ = 16;
  manager_ = std::make_unique<ContractManager>(
      std::make_unique<LevelDBStorage>(), worker_num_,
      ContractManager::Options::FX);
  // manager_ =
  // std::make_unique<ContractManager>(std::make_unique<DataStorage>(),
  // worker_num_, ContractManager::Options::OCC);
  address_manager_ = std::make_unique<AddressManager>();
  SetPreprocessFunc(
      [&](resdb::Request* request) { return this->ProcessRequest(request); });
}

int Thunderbolt::ProcessRequest(resdb::Request* request) {
  if (use_ov) {
    return 0;
  }
  // Execute the request, then send the response back to the user.
  BatchUserRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return 1;
  }

  std::vector<ContractExecuteInfo> contract_list;
  std::map<std::string, std::set<int>> ct;
  std::map<int, int> cct;

  std::vector<resdb::contract::Request> request_list;
  // LOG(ERROR)<<"request size:"<<batch_request.user_requests().size();
  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    request_list.push_back(request);
    if (!request_list.back().ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      continue;
    }
    request = request_list.back();

    if (request.cmd() == Request::CREATE_ACCOUNT) {
      auto new_req = batch_request.mutable_user_requests(i);
      //*new_req = sub_request;
      LOG(ERROR) << "create account:" << request.caller_address();
      std::lock_guard<std::mutex> lk(mutex_);
      absl::StatusOr<Account> account_or =
          ExecutionHelper::CreateAccountWithAddress(request.caller_address(),
                                                    address_manager_.get());
      if (account_or.ok()) {
        request.mutable_resp()->mutable_account()->Swap(&(*account_or));
      } else {
        request.mutable_resp()->set_ret(-1);
      }
      LOG(ERROR) << "get account:" << request.resp().account().address();
      request.SerializeToString(new_req->mutable_request()->mutable_data());
      continue;
    } else if (request.cmd() == Request::DEPLOY) {
      auto new_req = batch_request.mutable_user_requests(i);
      // auto new_req = new_batch_request.add_user_requests();
      std::lock_guard<std::mutex> lk(mutex_);
      absl::StatusOr<Contract> contract_or = ExecutionHelper::Deploy(
          request, address_manager_.get(), manager_.get());
      if (contract_or.ok()) {
        request.mutable_resp()->mutable_contract()->Swap(&(*contract_or));
      } else {
        request.mutable_resp()->set_ret(-1);
      }
      request.SerializeToString(new_req->mutable_request()->mutable_data());
      continue;
    } else if (request.cmd() == Request::EXECUTE) {
      contract_list.push_back(
          *ExecutionHelper::GetContractInfo(request, address_manager_.get()));
      contract_list.back().user_id = i;
    }
  }

  if (contract_list.empty()) {
    if (!batch_request.SerializeToString(request->mutable_data())) {
      LOG(ERROR) << "parse data fail";
      return 1;
    }
    LOG(ERROR) << "no transaction to exe";
    return 0;
  }

  std::vector<std::unique_ptr<ExecuteResp>> resp;
  {
    std::lock_guard<std::mutex> lk(mutex_);
    uint64_t time = GetCurrentTime();
    resp = manager_->ExecContract(contract_list);
    // LOG(ERROR)<<"contract list size:"<<contract_list.size()<<" resp
    // size:"<<resp.size(); LOG(ERROR)<<"run:"<<GetCurrentTime() - time;
  }

  std::map<int, std::unique_ptr<ExecuteResp>> resp_idx;
  for (int i = 0; i < resp.size(); ++i) {
    int id = resp[i]->user_id;
    resp[i]->rank = i;
    resp_idx[id] = std::move(resp[i]);
  }

  for (const auto& it : resp_idx) {
    int id = it.first;
    ExecuteResp* resp = it.second.get();

    auto* sub_request = batch_request.mutable_user_requests(id);

    ResultInfo info;
    info.mutable_resp()->set_res(resp->result);
    info.set_idx(resp->rank);

    // LOG(ERROR)<<"get rws size:"<<resp->rws.size();
    for (auto& rws_item : resp->rws) {
      const uint256_t& address = rws_item.first;
      const uint8_t* bytes = intx::as_bytes(address);
      size_t sz = sizeof(address);

      for (auto& data : rws_item.second) {
        RWS* new_rws = info.add_rws();
        // RWS * new_rws = request_list[id].add_rws();
        if (data.state == STORE) {
          new_rws->set_type(RWS::WRITE);
        } else {
          new_rws->set_type(RWS::READ);
        }
        new_rws->set_address(bytes, sz);
        const uint8_t* v_bytes = intx::as_bytes(data.data);
        size_t sz = sizeof(data.data);
        new_rws->set_value(v_bytes, sz);
        new_rws->set_version(data.version);
      }
    }

    info.SerializeToString(sub_request->mutable_request()->mutable_ext_data());
  }

  if (!batch_request.SerializeToString(request->mutable_data())) {
    LOG(ERROR) << "parse data fail";
    return 1;
  }
  // LOG(ERROR)<<"request done:";
  return 0;
}

}  // namespace contract
}  // namespace resdb
