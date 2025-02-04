//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "service/contract/benchmark/ycsb/ycsb_benchmark.h"

#include <cstring>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "core/client.h"
#include "core/core_workload.h"
#include "core/timer.h"
#include "core/utils.h"
#include "service/contract/benchmark/ycsb/db/contract_db.h"
#include "service/contract/benchmark/ycsb/ycsb_workload.h"

namespace resdb {
namespace contract {
namespace benchmark {
namespace ycsb {

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);

void PrintResult(const std::vector<BenchmarkResult> &results) {
  BenchmarkResult avg_res = results[0];

  double ktps_sum = 0;
  double latency_sum = 0;
  double retry_sum = 0;

  for (BenchmarkResult result : results) {
    ktps_sum += result.ktps;
    latency_sum += result.latency_s;
    retry_sum += result.average_retry;
    avg_res.max_retry = max(result.max_retry, avg_res.max_retry);
  }

  cout << " total op: " << avg_res.total_ops << " KTPS: " << avg_res.ktps
       << " latency size: " << avg_res.latency_size
       << " avg (s): " << avg_res.latency_s
       << " avg retry: " << avg_res.average_retry
       << " max retry: " << avg_res.max_retry << " db name: " << avg_res.db_name
       << " theta: " << avg_res.theta << " batch size: " << avg_res.batch_size
       << " threads: " << avg_res.thread_count << std::endl;
}

int DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops,
                   bool is_loading) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  int oks = 0;
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }
  db->Close();
  return oks;
}

string ParseCommandLine(int argc, const char *argv[],
                        utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-worker") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("worker_num", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-theta") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("theta", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-batchsize") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("batch_size", argv[argindex]);
      argindex++;

    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)"
       << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple "
          "files can"
       << endl;
  cout << "                   be specified, and will be processed in the order "
          "specified"
       << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

BenchmarkResult Process(utils::Properties props,
                        std::unique_ptr<ycsbc::DB> db) {
  /*
    std::unique_ptr<ycsbc::DB> db =
    resdb::contract::benchmark::OCCContractDB::CreateDB(props); if (!db) { cout
    << "Unknown database name " << props["dbname"] << endl; exit(0);
    }
    */

  resdb::contract::benchmark::YCSBWorkload wl;
  wl.Init(props);

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));
  vector<future<int>> actual_ops;
  int sum = 0;

  // Peforms transactions
  int total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  utils::Timer<double> timer;
  timer.Start();
  for (int i = 0; i < num_threads; ++i) {
    actual_ops.emplace_back(async(launch::async, DelegateClient, db.get(), &wl,
                                  total_ops / num_threads, false));
  }
  assert((int)actual_ops.size() == num_threads);

  sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }
  dynamic_cast<resdb::contract::benchmark::ContractDB *>(db.get())->Wait();
  double duration = timer.End();
  std::vector<uint64_t> latencys =
      dynamic_cast<resdb::contract::benchmark::ContractDB *>(db.get())
          ->GetLatencys();
  sort(latencys.begin(), latencys.end());
  uint64_t s = 0;
  for (size_t i = 0; i < latencys.size(); ++i) {
    s += latencys[i];
  }

  std::vector<int> retries =
      dynamic_cast<resdb::contract::benchmark::ContractDB *>(db.get())
          ->GetRetryTime();
  sort(retries.begin(), retries.end());
  uint64_t rty = 0;
  for (size_t i = 0; i < retries.size(); ++i) {
    rty += retries[i];
  }

  BenchmarkResult result;
  result.total_ops = total_ops;
  result.ktps = total_ops / duration / 1000;
  result.latency_size = latencys.size();
  result.latency_s = ((double)s / latencys.size() / 1000000);
  result.average_retry = retries[retries.size() - 1];
  result.average_retry = (double)rty / retries.size();
  result.max_retry = retries[retries.size() - 1];
  result.db_name = props["dbname"];
  result.theta = props["theta"];
  result.batch_size = props["batch_size"];
  result.thread_count = props["threadcount"];

  return result;
}

}  // namespace ycsb

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
