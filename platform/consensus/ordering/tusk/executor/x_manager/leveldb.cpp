#include "service/contract/executor/manager/leveldb.h"

#include "glog/logging.h"


namespace resdb {
namespace contract {

LevelDB::LevelDB(){
  db_ = std::make_unique<ResLevelDB>("./");
  db_->SetBatchSize(10000);
}

void LevelDB::Flush(){
  //LOG(ERROR)<<"flush";
  for(const auto& it : s){
    std::string addr = eevm::to_hex_string(it.first);
    std::string value = eevm::to_hex_string(it.second.first);
    //LOG(ERROR)<<"addr:"<<addr<<" value:"<<value;
    char * buf = new char[value.size()+ sizeof(it.second.second)];
    memcpy(buf, &it.second.second, sizeof(it.second.second));
    memcpy(buf+sizeof(it.second.second), value.c_str(), value.size());

    db_->SetValue(addr, std::string(buf, value.size()+sizeof(it.second.second)));
    delete buf;
  }
}

}
} // namespace eevm
