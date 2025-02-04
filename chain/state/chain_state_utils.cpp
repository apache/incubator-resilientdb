
#include "chain/state/chain_state_utils.h"

#ifdef ENABLE_LEVELDB
#include "chain/storage/res_leveldb.h"
#endif

namespace resdb {

std::unique_ptr<ChainState> NewState(const std::string& cert_file,
                                     const ResConfigData& config_data) {
  std::unique_ptr<Storage> storage = nullptr;

#ifdef ENABLE_ROCKSDB
  storage = NewResRocksDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use rocksdb storage.";
#endif

#ifdef ENABLE_LEVELDB
  storage = NewResLevelDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use leveldb storage.";
#endif
  std::unique_ptr<ChainState> state =
      std::make_unique<ChainState>(std::move(storage));
  return state;
}

}  // namespace resdb
