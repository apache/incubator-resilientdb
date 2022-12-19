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

#include "execution/geo_global_executor.h"

#include <glog/logging.h>

namespace resdb {

GeoGlobalExecutor::GeoGlobalExecutor(
    std::unique_ptr<TransactionExecutorImpl> geo_executor_impl,
    const ResDBConfig& config)
    : geo_executor_impl_(std::move(geo_executor_impl)),
      config_(config),
      is_stop_(false) {
  global_stats_ = Stats::GetGlobalStats();
  region_size_ = config_.GetConfigData().region().size();
  order_thread_ = std::thread(&GeoGlobalExecutor::OrderRound, this);
  my_region_ = config.GetConfigData().self_region_id();
}

GeoGlobalExecutor::~GeoGlobalExecutor() { Stop(); }

void GeoGlobalExecutor::Stop() {
  is_stop_ = true;
  if (order_thread_.joinable()) {
    order_thread_.join();
  }
}

void GeoGlobalExecutor::Execute(std::unique_ptr<Request> request) {
  BatchClientRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "[GeoGlobalExecutor] parse data fail!";
    return;
  }
  if (request->data().empty()) {
    LOG(ERROR) << "[GeoGlobalExecutor] request->data() is empty ";
    return;
  }

  if (global_stats_) {
    global_stats_->IncTotalGeoRequest(batch_request.client_requests_size());
  }
  if (geo_executor_impl_) {
    auto batch_response = geo_executor_impl_->ExecuteBatch(batch_request);
    if (request->region_info().region_id() == my_region_) {
      batch_response->set_createtime(batch_request.createtime());
      batch_response->set_local_id(batch_request.local_id());
      batch_response->set_proxy_id(batch_request.proxy_id());
      batch_response->set_seq(batch_request.seq());
      resp_queue_.Push(std::move(batch_response));
    }
  }
}

bool GeoGlobalExecutor::IsStop() { return is_stop_; }

int GeoGlobalExecutor::OrderGeoRequest(std::unique_ptr<Request> request) {
  order_queue_.Push(std::move(request));
  return 0;
}

void GeoGlobalExecutor::AddData() {
  auto request = order_queue_.Pop();
  if (request == nullptr) {
    return;
  }
  global_stats_->IncGeoRequest();
  uint64_t seq_num = request->seq();
  int region_id = request->region_info().region_id();
  execute_map_[std::make_pair(seq_num, region_id)] = std::move(request);
}

std::unique_ptr<Request> GeoGlobalExecutor::GetNextMap() {
  if (execute_map_.empty()) {
    return nullptr;
  }
  // LOG(ERROR) << "Next round seq: " << next_seq_ << ", current First in map: "
  // << execute_map_.begin()->first << ", size: " <<

  // LOG(ERROR)<<"get next seq:"<<next_seq_<<" region:"<<next_region_;
  if (execute_map_.begin()->first == std::make_pair(next_seq_, next_region_)) {
    std::unique_ptr<Request> res = std::move(execute_map_.begin()->second);
    execute_map_.erase(execute_map_.begin());
    next_region_++;
    if (next_region_ > region_size_) {
      next_region_ = 1;
      next_seq_++;
    }
    return res;
  }
  return nullptr;
}

void GeoGlobalExecutor::OrderRound() {
  while (!IsStop()) {
    AddData();
    while (!IsStop()) {
      std::unique_ptr<Request> seq_map = GetNextMap();
      if (seq_map == nullptr) {
        break;
      }
      Execute(std::move(seq_map));
    }
  }
}

std::unique_ptr<BatchClientResponse> GeoGlobalExecutor::GetResponseMsg() {
  return resp_queue_.Pop();
}

}  // namespace resdb
