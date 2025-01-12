#include <lmdb++.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

int main() {
  /* Create and open the LMDB environment: */
  auto env = lmdb::env::create();
  std::stringstream ss;
  env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL);
  env.open("./example.mdb", 0, 0664);

  /* Insert some key/value pairs in a write transaction: */
  auto wtxn = lmdb::txn::begin(env);
  auto dbi = lmdb::dbi::open(wtxn, nullptr);
  dbi.put(wtxn, "username", "jhacker");
  dbi.put(wtxn, "email", "jhacker@example.org");
  dbi.put(wtxn, "fullname", "J. Random Hacker");
  wtxn.commit();

  /* Fetch key/value pairs in a read-only transaction: */
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto cursor = lmdb::cursor::open(rtxn, dbi);
  std::string key, value;
  while (cursor.get(key, value, MDB_NEXT)) {
    std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
  }
  cursor.close();
  rtxn.abort();

  /* The enviroment is closed automatically. */

  return EXIT_SUCCESS;
}

int createDBPath(std::string directory_path = "example.mdb") {
  if (fs::exists(directory_path)) {
    std::cout << "Directory already exists: " << directory_path << std::endl;
    return 1;
  } else {
    if (fs::create_directory(directory_path)) {
      std::cout << "Directory created successfully: " << directory_path
                << std::endl;
      return 1;
    } else {
      std::cerr << "Failed to create directory: " << directory_path
                << std::endl;
      return 0;
    }
  }
}