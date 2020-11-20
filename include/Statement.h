//
// Created by fred.nicolson on 21/02/2020.
//

#ifndef TESTDB_STATEMENT_H
#define TESTDB_STATEMENT_H


#include <string>
#include <vector>

struct Statement
{
    Lexer::Token::Type query_type;
    std::vector<std::string> strings;
    std::string table_name;
    std::string compiled_where_clause;
    std::vector<std::string> compiled_result_clauses;
};


#endif //TESTDB_STATEMENT_H
