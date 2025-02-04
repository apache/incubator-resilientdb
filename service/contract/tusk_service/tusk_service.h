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

#include "platform/consensus/ordering/tusk/framework/consensus.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_manager.h"
#include "service/contract/proto/func_params.pb.h"
#include "service/contract/proto/rpc.pb.h"

namespace resdb {
namespace contract {

class Tusk : public tusk::TuskConsensus {
 public:
  Tusk(const ResDBConfig& config,
       std::unique_ptr<resdb::TransactionManager> executor);

  int ProcessRequest(resdb::Request* request);

 private:
  /*
    ContractExecuteInfo GetContractInfo(const resdb::contract::Request
   &request);
    //ConcurrencyController :: ModifyMap GetRWSList(const
   resdb::contract::ResultInfo &request);

   absl::StatusOr<Contract> Deploy(const resdb::contract::Request& request);
   absl::StatusOr<Account> CreateAccount();
   absl::StatusOr<Account> CreateAccountWithAddress(const std::string& address)
   ;

   bool DeployWithAddress(const resdb::contract::Request& request);
   bool CreateAccount(const std::string& address) ;
   */

 private:
  std::unique_ptr<resdb::contract::x_manager::ContractManager> manager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> address_manager_;

  std::unique_ptr<resdb::contract::x_manager::ContractManager> emanager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> eaddress_manager_;
  std::mutex mutex_;
  int worker_num_;
};

}  // namespace contract
}  // namespace resdb
