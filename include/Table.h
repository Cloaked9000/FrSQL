//
// Created by fred.nicolson on 11/03/2020.
//

#ifndef TESTDB_TABLE_H
#define TESTDB_TABLE_H


#include <unordered_map>
#include <vector>
#include <algorithm>
#include "Variable.h"

class Table
{
public:

    [[nodiscard]] const Variable &load_col(size_t offset, size_t index) const
    {
        return cols[offset][index];
    }

    [[nodiscard]] inline const std::vector<Variable> &load_row(size_t offset) const
    {
        return cols[offset];
    }

    [[nodiscard]] inline size_t row_count() const
    {
        return cols.size();
    }

    inline void insert(std::vector<Variable> values)
    {
        cols.emplace_back(std::move(values));
    }

    inline std::string_view get_column_name(size_t col)
    {
        return col_names.at(col);
    }

    inline size_t get_column_count()
    {
        return col_names.size();
    }

    inline std::optional<size_t> get_column_index(std::string_view name)
    {
        for(size_t a = 0; a < col_names.size(); ++a)
        {
            if(col_names[a] == name)
            {
                return a;
            }
        }

        return {};
    }



private:

    std::vector<std::vector<Variable>> cols = {{Variable(20), Variable("Bob", 3)}, {Variable(50), Variable("Dave", 4)}};
    std::vector<std::string_view> col_names = {"age", "name"};
};


#endif //TESTDB_TABLE_H
