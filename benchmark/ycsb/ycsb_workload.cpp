#include "benchmark/ycsb/ycsb_workload.h"

#include "core/scrambled_zipfian_generator.h"

namespace resdb {
namespace benchmark {

void YCSBWorkload::Init(const utils::Properties &p) {
  CoreWorkload::Init(p);

  std::string request_dist = p.GetProperty(REQUEST_DISTRIBUTION_PROPERTY,
                                           REQUEST_DISTRIBUTION_DEFAULT);
  double insert_proportion = std::stod(p.GetProperty(INSERT_PROPORTION_PROPERTY,
                                                     INSERT_PROPORTION_DEFAULT));
  double theta = std::stod(p.GetProperty("theta","0.99"));

  if (request_dist == "zipfian") {
    // If the number of keys changes, we don't want to change popular keys.
    // So we construct the scrambled zipfian generator with a keyspace
    // that is larger than what exists at the beginning of the test.
    // If the generator picks a key that is not inserted yet, we just ignore it
    // and pick another key.
    int op_count = std::stoi(p.GetProperty(OPERATION_COUNT_PROPERTY));
    int new_keys = (int)(op_count * insert_proportion * 2); // a fudge factor
    key_chooser_ = new ycsbc::ScrambledZipfianGenerator(0, record_count_ + new_keys-1,theta);
  } 
}

}
}