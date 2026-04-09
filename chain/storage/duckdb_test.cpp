#include "chain/storage/duckdb.h"
#include "chain/storage/proto/duckdb_config.pb.h"

#include <iostream>
#include <iterator>
#include <memory>
#include <string>

int main() {
    using namespace resdb::storage;

    // 1. Create a DuckDBInfo proto for file-backed DB
    DuckDBInfo config;
    // COmmented, when testing it set path according to your database, as provided below:
    // config.set_path("/home/namespace/resilientdb-duckdb/dtest/dtest.db");

    // 2. Construct DuckDB instance
    std::unique_ptr<DuckDB> my_db;
    try {
        my_db = std::make_unique<DuckDB>(config);
        std::cout << "DuckDB created successfully!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Failed to create DuckDB: " << e.what() << std::endl;
        return 1;
    }
    
    {
        std::string create_sql =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER, "
            "name TEXT"
            ");";

        std::string resp = my_db->ExecuteSQL(create_sql);
        std::cout << "CREATE: " << resp << std::endl;
    }

    {
        std::string insert_sql =
            "INSERT INTO users VALUES (1, 'batman');";

        std::string resp = my_db->ExecuteSQL(insert_sql);
        std::cout << "INSERT: " << resp << std::endl;
    }

    {
        std::string select_sql = "SELECT * FROM users;";

        std::string resp = my_db->ExecuteSQL(select_sql);
        std::cout << "SELECT: " << resp << std::endl;
    }

    return 0;
}
