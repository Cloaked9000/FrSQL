//
// Created by fred.nicolson on 21/02/2020.
//

#ifndef TESTDB_STATEMENT_H
#define TESTDB_STATEMENT_H


#include <string>
#include <utility>
#include <vector>

#include "TableMetadata.h"
#include "Lexer.h"

struct Statement
{
    enum class Order
    {
        Ascending = 0,
        Descending
    };

    void reset()
    {
        table_id = ID_NONE;
        compiled_where_clause.clear();
        compiled_limit_clause.clear();
        compiled_result_clauses.clear();
        compiled_insert_values_clause.clear();
        compiled_update_clause.clear();
        compiled_ordering_clause.clear();
        strings.clear();
        nested_statements.clear();
        column_definitions.clear();
        new_table_name.clear();
        column_redirect.clear();
        accessed_columns.clear();
        column_ids.clear();
        rows_returned = 0;
        order = Order::Ascending;
    }

    Lexer::Token::Type query_type;
    tid_t table_id = ID_NONE;

    std::string compiled_where_clause;
    std::string compiled_limit_clause;
    std::string compiled_result_clauses;
    std::string compiled_update_clause;
    std::string compiled_ordering_clause;

    std::vector<cid_t> column_ids;

    std::vector<std::string_view> strings;
    std::string compiled_insert_values_clause;
    std::vector<Statement> nested_statements;

    std::vector<ColumnMetadata> column_definitions;
    std::string new_table_name;

    std::vector<size_t> column_redirect;
    std::vector<std::string> accessed_columns;

    size_t rows_returned;
    Order order;
};


#endif //TESTDB_STATEMENT_H
