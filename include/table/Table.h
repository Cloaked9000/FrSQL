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
        auto data_index = Filesystem::Handle(filesystem, filesystem->open(metadata.name + ".index", true));
        auto data_data = Filesystem::Handle(filesystem, filesystem->open(metadata.name + ".data", true));
        auto index_tree = Filesystem::Handle(filesystem, filesystem->open(metadata.name + ".tree", true));
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

/*
class InMemoryTable : public Table
{
public:

    explicit InMemoryTable(TableMetadata meta)
    : Table(std::move(meta))
    {

    }

    [[nodiscard]] rid_t insert() override
    {
        rows.emplace_back();
        for (auto& col : get_metadata().columns)
        {
            rows.back().column.emplace_back();
            rows.back().column.back().type = col.type;
            switch(col.type)
            {
                case Variable::Type::INT:
                    rows.back().column.back().store.int64 = 0;
                    break;
                case Variable::Type::STRING:
                    strings.emplace_back("", 0);
                    rows.back().column.back().store.str = strings.back().data();
                    break;
                default:
                    abort();
            }
        }

        return rows.size() - 1;
    }

    void erase(rid_t row_id) override
    {
        //If string, remove from string store
        for(auto &col : rows[row_id].column)
        {
            if(col.type != Variable::Type::STRING)
            {
                continue;
            }

            auto iter = std::find_if(strings.begin(), strings.end(), [addr = col.store.str](const auto &str) {
                return str.data() == addr;
            });

            if(iter == strings.end())
            {
                // String data is missing?!?
                abort();
            }

            strings.erase(iter);
        }



        rows.erase(rows.begin() + row_id);
    }

    [[nodiscard]] Variable load(rid_t row_id, cid_t col_id) override
    {
        return Variable(rows[row_id].column[col_id]);
    }

    void update(rid_t row_id, cid_t col_id, Variable new_val) override
    {
        Variable &old_val = rows[row_id].column[col_id];
        if(old_val.type != new_val.type)
        {
            throw SemanticError("New column value is of different type to old!");
        }

        // If it's a string and we're overwriting it, update string store
        if(old_val.type == Variable::Type::STRING)
        {
            auto iter = std::find_if(strings.begin(), strings.end(), [addr = old_val.store.str](const auto &str) {
                return str.data() == addr;
            });

            if(iter == strings.end())
            {
                // String data is missing?!?
                abort();
            }

            // Update to contain new string
            *iter = std::string(new_val.store.str, new_val.store.len);
        }

        // Store new data
        old_val = new_val;
    }

    void clear() override
    {
        strings.clear();
        rows.clear();
    }

    [[nodiscard]] rid_t get_row_count() const override
    {
        return rows.size();
    }

private:
    struct Row
    {
        std::vector<Variable> column;
    };

    std::vector<Row> rows;
    std::list<std::string> strings;
};
*/
#endif //TESTDB_TABLE_H
