//
// Created by fred on 12/07/2020.
//

#include "StorageEngine.h"
#include "Table.h"

std::shared_ptr<Table> StorageEngine::load_table(std::string_view table_name)
{
    static std::shared_ptr<Table> table = std::make_shared<Table>();
    return table;
}

std::vector<std::string_view> StorageEngine::list_tables()
{
    static std::vector<std::string_view> tables = {"tab"};
    return tables;
}
