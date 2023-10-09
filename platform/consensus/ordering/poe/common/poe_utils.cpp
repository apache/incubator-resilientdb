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

#include "platform/consensus/ordering/poe/common/poe_utils.h"

namespace resdb {
namespace poe {

std::unique_ptr<Request> NewRequest(const POERequest& request,
                                    POERequest::Type type, int id) {
  POERequest new_request(request);
  new_request.set_type(type);
  new_request.set_sender_id(id);
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  return ret;
}

int32_t PrimaryId(int64_t view_num, int total_num) {
  view_num--;
  return (view_num % total_num) + 1;
}

std::unique_ptr<Request> ConvertToRequest(const POERequest& new_type) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_proxy_id(new_type.proxy_id());
  request->set_sender_id(new_type.sender_id());
  request->set_hash(new_type.hash());
  request->set_type(new_type.type());
  request->set_seq(new_type.seq());
  request->set_data(new_type.data());
  request->set_current_view(new_type.current_view());
  return request;
}

std::unique_ptr<POERequest> ConvertToCustomRequest(const Request& request) {
  std::unique_ptr<POERequest> user_request = std::make_unique<POERequest>();
  user_request->set_proxy_id(request.proxy_id());
  user_request->set_sender_id(request.sender_id());
  user_request->set_hash(request.hash());
  user_request->set_type(request.type());
  user_request->set_seq(request.seq());
  user_request->set_data(request.data());
  user_request->set_current_view(request.current_view());
  return user_request;
}

}  // namespace poe
}  // namespace resdb
