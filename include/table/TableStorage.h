//
// Created by fred on 17/07/2022.
//

#ifndef TESTDB_TABLESTORAGE_H
#define TESTDB_TABLESTORAGE_H


#include <optional>
#include "TableMetadata.h"
#include "filesystem/Filesystem.h"

class TableStorage
{
public:
    explicit TableStorage(Filesystem::Handle index);
    ~TableStorage();
    TableStorage(const TableStorage&)=delete;
    TableStorage(TableStorage&&)=delete;
    void operator=(TableStorage&&)=delete;
    void operator=(const TableStorage&)=delete;

    tid_t create(TableMetadata metadata);
    void erase(tid_t table_id);
    std::optional<tid_t> find(std::string_view name);
    std::vector<tid_t> list();
    TableMetadata load(tid_t table_id);
private:
    struct TableStorageMetadata
    {
        tid_t current_table_id = 0;
        tid_t table_count = 0;
    };

    Filesystem::Handle data;
    std::vector<TableMetadata> tables;
    TableStorageMetadata metadata;
};


#endif //TESTDB_TABLESTORAGE_H
