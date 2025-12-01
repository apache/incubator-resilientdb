#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>


#include "chain/storage/proto/duckdb_config.pb.h"

#include "storage.h"
#include "duckdb.hpp"

namespace resdb {
namespace storage {

class ResQL : public Storage {
 public:
  explicit ResQL(const DuckDBInfo& config);
  ~ResQL() override;

  // Main functionality
  std::string ExecuteSQL(const std::string& sql_string) override;
  void CreateDB(const DuckDBInfo& config);

  // No-op overrides
  int SetValue(const std::string&, const std::string&) override { return 0; }
  std::string GetValue(const std::string&) override { return ""; }
  std::string GetAllValues() override { return ""; }
  std::string GetRange(const std::string&, const std::string&) override {
    return "";
  }
  int SetValueWithVersion(const std::string&, const std::string&, int) override {
    return 0;
  }
  std::pair<std::string, int> GetValueWithVersion(const std::string&,
                                                  int) override {
    return {"", 0};
  }
  std::map<std::string, std::pair<std::string, int>> GetAllItems() override {
    return {};
  }
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string&, const std::string&) override {
    return {};
  }
  std::vector<std::pair<std::string, int>> GetHistory(const std::string&, int,
                                                      int) override {
    return {};
  }
  std::vector<std::pair<std::string, int>> GetTopHistory(const std::string&,
                                                         int) override {
    return {};
  }
  bool Flush() override { return true; }

 private:
  std::unique_ptr<duckdb::DuckDB> db_;
  std::unique_ptr<duckdb::Connection> conn_;
  std::optional<DuckDBInfo> config_;
};


// Factory function
std::unique_ptr<Storage> NewResQL(
    const std::string& path,
    const DuckDBInfo& config = DuckDBInfo());
}  // namespace storage
}  // namespace resdb
