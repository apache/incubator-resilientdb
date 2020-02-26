#include "database.h"
#include <unordered_map>
#include <iostream>

InMemoryDB::InMemoryDB()
{
    _dbInstance = "InMemory";
}

int InMemoryDB::Open(const std::string)
{
    db = new std::unordered_map<std::string, dbTable>();
    activeTable = "table1";

    std::cout << std::endl
              << "In-Memory DB configuration OK" << std::endl;

    return 0;
}

std::string InMemoryDB::Get(const std::string key)
{
    return (*db)[activeTable][key];
}

std::string InMemoryDB::Put(const std::string key, const std::string value)
{
    std::string oldValue = Get(key);
    (*db)[activeTable][key] = value;
    return oldValue;
}

int InMemoryDB::SelectTable(const std::string tableName)
{
    if (tableName == activeTable)
    {
        return 1;
    }
    activeTable = tableName;
    return 0;
}

int InMemoryDB::Close(const std::string)
{
    delete db;
    return 0;
}
