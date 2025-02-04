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

#include <thread>

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "service/contract/executor/common/contract_execute_info.h"
#include "service/contract/executor/common/utils.h"
#include "service/contract/executor/manager/concurrency_controller.h"
#include "service/contract/executor/manager/evm_state.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {

/*
struct ExecuteResp {
  int ret;
  absl::Status state;
  int64_t commit_id;
  Address contract_address;
    ConcurrencyController :: ModifyMap rws;
  std::string result;
  int retry_time = 0;
  uint64_t user_id = 0;
  double runtime = 0;
};
*/

class ContractExecutor {
 public:
  ContractExecutor() = default;

  ~ContractExecutor() = default;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

  absl::StatusOr<std::vector<uint8_t>> Execute(
      const Address& owner_address, const Address& contract_address,
      const std::vector<uint8_t>& func_params, EVMState* state);
};

}  // namespace contract
}  // namespace resdb
