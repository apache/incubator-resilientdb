#include "config.h"
#include "database.h"
#include "assert.h"
#include <iostream>

// Private methods

int SQLite::callback(void *x, int nCol, char **colValue, char **colNames)
{
    res_data *data = (res_data *)x;
    //std::cout << "DATA returned from " << data->op << " operation" << std::endl;

    if (data->results.size() == 0)
    {
        // Store column names for debugging
        data->results.push_back(std::vector<std::string>());
        for (int i = 0; i < nCol; i++)
        {
            data->results.back().push_back(colNames[i]);
        }
    }

    data->results.push_back(std::vector<std::string>());
    for (int i = 0; i < nCol; i++)
    {
        data->results.back().push_back(colValue[i] ? colValue[i] : "NULL");
    }

    return 0;
}

int SQLite::createTable(const std::string tableName)
{
    std::string query = "CREATE TABLE IF NOT EXISTS " + tableName + "("
                                                                    "KEY      CHAR(64) PRIMARY KEY    NOT NULL,"
                                                                    "VALUE    TEXT                    NOT NULL);";

    int rc = sqlite3_exec(*db, query.c_str(), this->callback, 0, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "query: " << query << std::endl;
        std::cerr << "[createTable]: SQL error " << rc << ": " << sqlite3_errmsg(*db) << std::endl;
        assert(0);
        return 1;
    }
    else
    {
        return 0;
    }
}

// Public methods

SQLite::SQLite()
{
    _dbInstance = "SQLite";
}

int SQLite::Open(const std::string dbName)
{
    //std::string sqliteName = dbName + "_SQLite";
    std::string sqliteName = dbName;
    #if EXT_DB == SQL_PERSISTENT
        int rc = sqlite3_open(sqliteName.c_str(), db);
        std::cout << std::endl << "SQlite Persistent.";
    #else
        int rc = sqlite3_open(":memory:", db);
        std::cout << std::endl << "SQlite In-Memory.";
    #endif

    if (rc != SQLITE_OK)
    {
        std::cerr << "Error opening SQLite3 database: " << sqlite3_errmsg(*db) << std::endl;
        sqlite3_close(*db);
        assert(0);
    }

    if (SelectTable("KV") != 0)
    {
        return rc;
    }

    std::cout << std::endl
              << "SQLite configuration OK" << std::endl;

    return rc;
}

std::string SQLite::Get(const std::string key)
{
    std::string value;
    res_data res("GET");

    std::string query = "SELECT KEY, VALUE FROM " + this->table + " WHERE KEY = '" + key + "';";
    int rc = sqlite3_exec(*db, query.c_str(), this->callback, (void *)&res, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "query: " << query << std::endl;
        std::cerr << "[GET]: SQL error " << rc << ": " << sqlite3_errmsg(*db) << std::endl;
        assert(0);
    }

    // Print all returned ROWS
    // First row contains the name of each returned columns
    // The next rows contain the actual values return
    //for(auto &row: res.res) {
    //    for(auto &col: row) {
    //        std::cout << col << " ";
    //    }
    //    std::cout << std::endl;
    //}

    if (res.results.size())
    {
        // skip first row that contains the COLUMN name
        // skip first row that contains the KEY and return VALUE
        value = res.results[1][1];
    }

    return value;
}

std::string SQLite::Put(const std::string key, const std::string value)
{
    std::string query;
    res_data res("PUT");

    std::string prev = Get(key);
    // If the value to be inserted is the same, just return
    if (value == prev)
    {
        return prev;
    }

    if (prev.size())
    {
        //std::cout << "Updating the value" << std::endl;
        query = "UPDATE " + this->table + " set VALUE = '" + value + "' where KEY = '" + key + "';";
    }
    else
    {
        //std::cout << "Inserting the value" << std::endl;
        query = "INSERT INTO " + table + " (KEY, VALUE) VALUES('" + key + "', '" + value + "');";
    }

    int rc = sqlite3_exec(*db, query.c_str(), this->callback, (void *)&res, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "query: " << query << std::endl;
        std::cerr << "[PUT]: Error executing SQLite query: " << sqlite3_errmsg(*db) << std::endl;
        assert(0);
    }

    return prev;
}

int SQLite::SelectTable(const std::string tableName)
{
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";
    res_data res("SelectTable");

    int rc = sqlite3_exec(*db, query.c_str(), this->callback, (void *)&res, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "query: " << query << std::endl;
        std::cerr << "[SelectTable]: Error " << rc << " executing SQLite query: " << sqlite3_errmsg(*db) << std::endl;
        assert(0);
        return 1;
    }

    this->table = tableName;

    if (res.results.size() == 0)
    {
        return this->createTable(tableName);
    }

    return 0;
}

int SQLite::Close(const std::string dbName)
{
    if (*db != nullptr)
    {
        delete db;
        return 0;
    }

    return 1;
}
