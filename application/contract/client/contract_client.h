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

#include "application/contract/proto/account.pb.h"
#include "application/contract/proto/contract.pb.h"
#include "client/resdb_user_client.h"

namespace resdb {
namespace contract {

// ContractClient to send data to the contract server.
class ContractClient : public ResDBUserClient {
 public:
  ContractClient(const ResDBConfig& config);

  absl::StatusOr<Account> CreateAccount();
  absl::StatusOr<Contract> DeployContract(
      const std::string& caller_address, const std::string& contract_name,
      const std::string& contract_path,
      const std::vector<std::string>& init_params);

  absl::StatusOr<std::string> ExecuteContract(
      const std::string& caller_address, const std::string& contract_address,
      const std::string& func_name,
      const std::vector<std::string>& func_params);
};

}  // namespace contract
}  // namespace resdb
