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

#pragma once

//#include "platform/config/resdb_config_utils.h"
#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_manager.h"
#include "service/contract/executor/x_manager/streaming_committer.h"
#include "service/contract/proto/func_params.pb.h"
#include "service/contract/proto/rpc.pb.h"

namespace resdb {
namespace contract {

class ContractTransactionManager : public TransactionManager {
 public:
  ContractTransactionManager(Storage* storage);
  virtual ~ContractTransactionManager() = default;

  virtual std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override;

  bool VerifyAndExecuteRequest(const BatchUserRequest& batch_request);

  std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
  Prepare(const BatchUserRequest& request) override;

  std::unique_ptr<BatchUserResponse> ExecutePreparedData(
      const BatchUserRequest& batch_request) override;

  bool ExecuteRequest(const BatchUserRequest& batch_request);
  void AsyncExe(std::unique_ptr<resdb::Request> request,
                const BatchUserRequest& batch_request);

 private:
  void Execute(const Request& request);

 private:
  std::unique_ptr<resdb::contract::x_manager::ContractManager> manager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> address_manager_;

  typedef std::map<int, std::pair<std::unique_ptr<ContractExecuteInfo>,
                                  std::unique_ptr<ModifyMap>>>
      DataType;
  std::map<int, DataType> data_;
  std::map<int, std::unique_ptr<resdb::Request>> req_seq_;
  std::unique_ptr<resdb::contract::x_manager::StreamingCommitter> dg_committer_;
  std::map<int, std::unique_ptr<BatchUserRequest>> batch_req_;

  std::mutex mutex_, req_mutex_;
};

}  // namespace contract
}  // namespace resdb
