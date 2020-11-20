//
// Created by fred on 12/07/2020.
//

#ifndef TESTDB_STORAGEENGINE_H
#define TESTDB_STORAGEENGINE_H
#include <memory>
#include <vector>
#include <string_view>

class Table;
class StorageEngine
{
public:
    std::shared_ptr<Table> load_table(std::string_view table_name);
    std::vector<std::string_view> list_tables();
};


#endif //TESTDB_STORAGEENGINE_H
