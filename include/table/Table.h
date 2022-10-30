//
// Created by fred.nicolson on 11/03/2020.
//

#ifndef TESTDB_TABLE_H
#define TESTDB_TABLE_H


#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <list>
#include "Variable.h"
#include "btree/BTree.h"
#include "filesystem/Filesystem.h"
#include "TableMetadata.h"
#include "RowStorage.h"

class Table
{
public:
    explicit Table(TableMetadata meta)
    : metadata(std::move(meta))
    {

    }

    [[nodiscard]] inline const TableMetadata &get_metadata() const
    {
        return metadata;
    }

    virtual ~Table()=default;
    [[nodiscard]] virtual rid_t insert(const std::vector<Variable> &values)=0;
    virtual void erase(rid_t row_id)=0;
    [[nodiscard]] virtual Variable load(rid_t row_id, cid_t col_id)=0;
    virtual void update(rid_t row_id, cid_t col_id, Variable value)=0;
    virtual void clear()=0;
    [[nodiscard]] virtual rid_t get_row_count() const=0;

protected:
    TableMetadata metadata;
};

class TreeTable : public Table
{
public:

    explicit TreeTable(TableMetadata meta, Filesystem *filesystem)
    : Table(std::move(meta)), filesystem(filesystem)
    {
        auto data_index = filesystem->open(metadata.name + ".index", true);
        auto data_data = filesystem->open(metadata.name + ".data", true);
        auto index_tree = filesystem->open(metadata.name + ".tree", true);
        row_store = std::make_unique<RowStorage>(metadata, std::move(data_index), std::move(data_data));
        index.open(std::move(index_tree));
    }

    [[nodiscard]] rid_t insert(const std::vector<Variable> &row) override
    {
        auto id = row_store->store(row);
        index.insert(current_rid + 1, id);
        return current_rid++;
    }

    void erase(rid_t row_id) override
    {

    }

    [[nodiscard]] Variable load(rid_t row_id, cid_t col_id) override
    {
        auto store_id = index.search(row_id + 1);
        if(!store_id)
        {
            throw DatabaseError("Row not found");
        }

        return row_store->load(store_id.value() - 1).at(col_id); //todo return whole thing
    }

    void update(rid_t row_id, cid_t col_id, Variable new_val) override
    {

    }

    void clear() override
    {

    }

    [[nodiscard]] rid_t get_row_count() const override
    {
        return metadata.get_column_count();
    }

private:
    std::unique_ptr<RowStorage> row_store;
    Filesystem *filesystem;
    rid_t current_rid = 0;
    Tree index;
};
#endif //TESTDB_TABLE_H
