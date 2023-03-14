#ifndef _DATABASE__
#define _DATABASE__

#include <string>
#include <unordered_map>
#include "sqlite3.h"
#include <vector>
/**
 * class DataBase
 * 
 * The abstract class DataBase provide an unified 
 * interface for managing any king of database 
 * that supports key-value pair
 */
class DataBase
{
protected:
    std::string _dbInstance;

public:
    /**
     * Open a database connection
     * @param id represents the id of connection
     * @return the status code of the operation, 0 if the operation was successful
     */
    virtual int Open(const std::string id = {}) = 0;

    /**
     * Get the value associate to the given key from the database
     * @param key represents a key in the database
     * @return the value associate to the key if the key is present or an empty string
     */
    virtual std::string Get(const std::string key) = 0;

    /**
     * Put a key-value pair in the database
     * @param key represents a key in the database
     * @param value represent the new value that should be associate to the key
     * @return the previous value associate to the key
     */
    virtual std::string Put(const std::string key, const std::string value) = 0;

    /**
     * Select a new active table for the database
     * @param tableName is the name of the table that will be activate
     * @return the status code of the operation, 0 if the operation was successful
     */
    virtual int SelectTable(const std::string tableName) = 0;

    /**
     * Close a database connection
     * @param id represents the id of connection
     * @return the status code of the operation, 0 if the operation was successful
     */
    virtual int Close(const std::string id = {}) = 0;

    /**
     * dbInstance return a string representing the 
     * type of the concrete database instace
     *
     * @return the db type
     */
    std::string dbInstance()
    {
        return _dbInstance;
    }
};

class SQLite : public DataBase
{
private:
    typedef struct _res_data
    {
        std::vector<std::vector<std::string>> results;
        std::string op;
        _res_data(const std::string &opType) : op(opType) {}
    } res_data;

    sqlite3 **db = new sqlite3 *();
    std::string table;
    int static callback(void *data, int nCol, char **colValue, char **colNames);
    int createTable(const std::string tableName);

public:
    SQLite();
    int Open(const std::string = "db");
    std::string Get(const std::string key);
    std::string Put(const std::string key, const std::string value);
    int SelectTable(const std::string tableName);
    int Close(const std::string = "db");
};


class InMemoryDB : public DataBase
{
private:
    using dbTable = std::unordered_map<std::string, std::string>;

    std::string activeTable;
    std::unordered_map<std::string, dbTable> *db;

public:
    InMemoryDB();
    int Open(const std::string = "db");
    std::string Get(const std::string key);
    std::string Put(const std::string key, const std::string value);
    int SelectTable(const std::string tableName);
    int Close(const std::string = "db");
};

#endif