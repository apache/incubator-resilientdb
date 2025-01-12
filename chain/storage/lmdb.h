#pragma once

#include <lmdb++.h>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "chain/storage/storage.h"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResLmdb(const std::string& path);

class LmdbStorage : public Storage {
 public:
  using ValueType = std::pair<std::string, int>;
  using ItemsType = std::map<std::string, ValueType>;
  using ValuesType = std::vector<ValueType>;
  /**
   * Constructor to initialize the LMDB environment and database.
   * @param db_path Path to the LMDB database file.
   * @param map_size Size of the LMDB map (default: 1GB).
   */
  LmdbStorage(const std::string& db_path, size_t map_size);

  /**
   * Destructor to clean up resources.
   */
  ~LmdbStorage();

  /**
   * Sets a key-value pair in the database.
   * @param key The key to store.
   * @param value The value associated with the key.
   * @return 0 on success, -1 on failure.
   */
  int SetValue(const std::string& key, const std::string& value) override;

  /**
   * Retrieves the value associated with a key.
   * @param key The key to retrieve.
   * @return The value, or an empty string if the key doesn't exist.
   */
  std::string GetValue(const std::string& key) override;

  /**
   * Retrieves all key-value pairs in the database.
   * @return A string representation of all key-value pairs.
   */
  std::string GetAllValues() override;

  /**
   * Retrieves all key-value pairs within a specified range.
   * @param start The start of the key range (inclusive).
   * @param end The end of the key range (inclusive).
   * @return A string representation of all key-value pairs in the range.
   */
  std::string GetRange(const std::string& start,
                       const std::string& end) override;

  /**
   * Sets a key-value pair with an associated version.
   * @param key The key to store.
   * @param value The value associated with the key.
   * @param version The version to associate with the value.
   * @return 0 on success, -1 on failure.
   */
  int SetValueWithVersion(const std::string& key, const std::string& value,
                          int version) override;

  /**
   * Retrieves a key-value pair along with its version.
   * @param key The key to retrieve.
   * @param version The version to retrieve.
   * @return A pair containing the value and version, or an empty value and -1
   * if not found.
   */
  ValueType GetValueWithVersion(const std::string& key, int version) override;

  /**
   * Flushes the database.
   * Note: LMDB writes directly to disk, so explicit flushing isn't required.
   * @return Always true.
   */

  std::map<std::string, std::pair<std::string, int>> GetAllItems() override;
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) override;

  // Return a list of <value, version>
  std::vector<std::pair<std::string, int>> GetHistory(const std::string& key,
                                                      int min_version,
                                                      int max_version) override;

  std::vector<std::pair<std::string, int>> GetTopHistory(
      const std::string& key, int top_number) override;

  bool Flush() override;

 private:
  lmdb::env env_;  ///< LMDB environment.
};

}  // namespace storage
}  // namespace resdb