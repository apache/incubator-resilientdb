#include "service/contract/benchmark/ycsb/occ_contract.h"
#include "service/contract/benchmark/ycsb/ycsb_benchmark.h"

using namespace std;
using namespace resdb::contract::benchmark::ycsb;

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  std::vector<BenchmarkResult> results;
  for (int i = 0; i < 1; ++i) {
    std::unique_ptr<ycsbc::DB> db =
        resdb::contract::benchmark::OCCContractDB::CreateDB(props);
    if (!db) {
      cout << "Unknown database name " << props["dbname"] << endl;
      exit(0);
    }
    results.push_back(Process(props, std::move(db)));
  }
  PrintResult(results);
}
