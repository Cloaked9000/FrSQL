//
// Created by fred on 19/03/2021.
//

#ifndef TESTDB_TESTUTILS_H
#define TESTDB_TESTUTILS_H

#include <gtest/gtest.h>
#include <frsql.h>

inline void PrintTo(const std::vector<row_t> &rows, ::std::ostream* os)
{
    *os << "{";
    for (size_t a = 0; a < rows.size(); a++)
    {
        *os << "{";
        for (size_t b = 0; b < rows[a].size(); b++)
        {
            if (rows[a][b].type == Variable::Type::INT)
            {
                *os << rows[a][b].store.int64;
            }
            else if (rows[a][b].type == Variable::Type::STRING)
            {
                *os << std::string_view(rows[a][b].store.str, rows[a][b].store.len);
            }
            if (b != rows[a].size() - 1)
            {
                *os << ", ";
            }
        }
        if (a != rows.size() - 1)
        {
            *os << ", ";
        }
        *os << "}";
    }
    *os << "}";
}

#endif //TESTDB_TESTUTILS_H
