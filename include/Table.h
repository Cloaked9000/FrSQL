//
// Created by fred.nicolson on 11/03/2020.
//

#ifndef TESTDB_TABLE_H
#define TESTDB_TABLE_H


#include <unordered_map>
#include <vector>
#include <optional>
#include <algorithm>
#include <list>
#include "Variable.h"
#include "TableMetadata.h"

// Note: this whole file is placeholder
class Table
{
public:
    explicit Table(TableMetadata metadata)
    : metadata(std::move(metadata))
    {

    }

    void set_col(size_t column_id, size_t row_id, const Variable &var)
    {
        Variable &old = cols.at(row_id).at(column_id);

        if(old.type != var.type)
        {
            throw SemanticError("New column value is of different type to old!");
        }

        // If it's a string and we're overwriting it, update string store
        if(old.type == Variable::Type::STRING)
        {
            auto iter = std::find_if(strings.begin(), strings.end(), [addr = old.store.str](const auto &str) {
               return str.data() == addr;
            });
            if(iter == strings.end())
            {
                // String data is missing?!?
                abort();
            }

            // Update to contain new string
            *iter = std::string(var.store.str, var.store.len);
        }

        // Store new data
        old = var;
    }

    [[nodiscard]] const Variable &load_col(size_t offset, size_t index) const
    {
        return cols.at(offset).at(index);
    }

    [[nodiscard]] inline const std::vector<Variable> &load_row(size_t offset) const
    {
        return cols[offset];
    }

    void inline delete_row(size_t offset)
    {
        cols.erase(cols.begin() + offset);
    }

    [[nodiscard]] inline size_t row_count() const
    {
        return cols.size();
    }

    inline void insert(std::vector<Variable> values)
    {
        for (auto& val : values)
        {
            if (val.type == Variable::Type::STRING)
            {
                strings.emplace_back(val.store.str, val.store.len);
                val.store.str = strings.back().data();
            }
        }
        cols.emplace_back(std::move(values));
    }

    inline std::string_view get_column_name(size_t col)
    {
        return metadata.columns.at(col).name;
    }

    [[nodiscard]] inline size_t get_column_count() const
    {
        return metadata.columns.size();
    }

    inline std::optional<size_t> get_column_index(std::string_view name)
    {
        for(size_t a = 0; a < metadata.columns.size(); ++a)
        {
            if(metadata.columns[a].name == name)
            {
                return std::make_optional(a);
            }
        }

        return {};
    }

    inline const TableMetadata& get_metadata()
    {
        return metadata;
    }

    void clear()
    {
        strings.clear();
        for (auto& col : cols)
        {
            col.clear();
        }
    }

private:
    TableMetadata metadata;
    std::vector<std::vector<Variable>> cols;
    std::list<std::string> strings;
};


#endif //TESTDB_TABLE_H
