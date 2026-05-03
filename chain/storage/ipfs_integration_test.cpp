#include <glog/logging.h>
#include <iostream>
#include "chain/storage/ipfs_client.h"
#include "chain/storage/proto/ipfs_config.pb.h"

using namespace resdb::storage;

int main() {
  google::InitGoogleLogging("ipfs_test");
  FLAGS_minloglevel = 0;

  IPFSConfig config;
  config.set_api_endpoint("127.0.0.1:5001");
  config.set_enabled(true);
  config.set_gateway_endpoint("127.0.0.1:8080");
  config.set_timeout_ms(30000);
  config.set_max_retries(3);

  auto client = IPFSClient::Create(config);
  std::cout << "IPFS client created, enabled=" << client->IsEnabled() << std::endl;

  std::string cid = client->Add("hello_tiered_storage");
  std::cout << "Add returned CID: " << cid << std::endl;

  std::string data = client->Cat(cid);
  std::cout << "Cat returned: " << data << std::endl;

  bool exists = client->Exists(cid);
  std::cout << "Exists(" << cid << ")=" << (exists ? "true" : "false") << std::endl;

  std::string dag_cid = client->AddDAG("{\"key\":\"test_key_1\",\"value\":\"hello_tiered_storage\"}");
  std::cout << "AddDAG returned CID: " << dag_cid << std::endl;

  std::string dag_data = client->GetDAG(dag_cid);
  std::cout << "GetDAG returned: " << dag_data << std::endl;

  std::cout << "IPFS test complete!" << std::endl;
  return 0;
}
