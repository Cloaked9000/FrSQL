//
// Created by fred on 29/06/2022.
//

#ifndef TESTDB_ROWSTORAGE_H
#define TESTDB_ROWSTORAGE_H


#include "filesystem/Filesystem.h"
#include "TableMetadata.h"
#include "Variable.h"

class RowStorage
{
public:
    RowStorage(TableMetadata metadata, Filesystem::Handle index, Filesystem::Handle data);

    rid_t store(const std::vector<Variable> &row);
    void erase(rid_t row_id);
    std::vector<Variable> load(rid_t row_id);
    void update(rid_t row_id, const std::vector<Variable> &row);
private:
    size_t row_size;
    TableMetadata metadata;
    Filesystem::Handle index;
    Filesystem::Handle data;
};


#endif //TESTDB_ROWSTORAGE_H
