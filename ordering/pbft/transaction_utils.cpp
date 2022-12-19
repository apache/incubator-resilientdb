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

#include "ordering/pbft/transaction_utils.h"

namespace resdb {

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id) {
  auto new_request = std::make_unique<Request>(request);
  new_request->set_type(type);
  new_request->set_sender_id(sender_id);
  return new_request;
}

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id, int region_id) {
  auto new_request = std::make_unique<Request>(request);
  new_request->set_type(type);
  new_request->set_sender_id(sender_id);
  new_request->mutable_region_info()->set_region_id(region_id);
  return new_request;
}

}  // namespace resdb
