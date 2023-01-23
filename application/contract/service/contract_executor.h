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

#include "application/contract/manager/address_manager.h"
#include "application/contract/manager/contract_manager.h"
#include "application/contract/proto/func_params.pb.h"
#include "application/contract/proto/rpc.pb.h"
#include "config/resdb_config_utils.h"
#include "execution/transaction_executor_impl.h"

namespace resdb {
namespace contract {

class ContractExecutor : public TransactionExecutorImpl {
 public:
  ContractExecutor(void);
  virtual ~ContractExecutor() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 private:
  absl::StatusOr<Account> CreateAccount();
  absl::StatusOr<Contract> Deploy(const Request& request);
  absl::StatusOr<std::string> Execute(const Request& request);

 private:
  std::unique_ptr<ContractManager> contract_manager_;
  std::unique_ptr<AddressManager> address_manager_;
};

}  // namespace contract
}  // namespace resdb
