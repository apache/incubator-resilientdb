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

#include "platform/consensus/ordering/tusk/proto/tusk.pb.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

template <typename Type>
std::unique_ptr<Request> NewRequest(const Type& request, TuskRequest::Type type,
                                    int id);

template <typename Type>
std::unique_ptr<Request> NewRequest(const Type& request, TuskRequest::Type type,
                                    int id) {
  Type new_request(request);
  new_request.set_sender_id(id);
  new_request.set_type(type);
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  ret->set_type(static_cast<TuskRequest::Type>(type));
  return ret;
}

}  // namespace resdb
