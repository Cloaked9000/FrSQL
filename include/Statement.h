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
    Lexer::Token::Type query_type;
    tid_t table_id = ID_NONE;

    std::string compiled_where_clause;
    std::string compiled_limit_clause;
    std::string compiled_result_clauses;

    std::vector<std::string_view> strings;
    std::string compiled_insert_values_clause;
    std::vector<Statement> nested_statements;

    std::vector<ColumnMetadata> column_definitions;
    std::string new_table_name;

    std::vector<size_t> column_redirect;
    std::vector<std::string> accessed_columns;

    size_t evaluated_limit;
    size_t rows_returned;
};


#endif //TESTDB_STATEMENT_H
