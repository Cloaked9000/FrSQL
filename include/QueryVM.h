//
// Created by fred.nicolson on 21/02/2020.
//

#ifndef TESTDB_QUERYVM_H
#define TESTDB_QUERYVM_H

#include "Variable.h"

class Table;
class Statement;
typedef std::vector<Variable> row_t;
class QueryVM
{
public:
    bool fetch_row(row_t *row);
    void eval_stmt(Statement *stmt);

private:
    void eval_select(Statement *stmt);
    void exec(Statement *stmt, std::string_view bytecode);

    template<typename ...Args>
    inline void push(Args &&...args)
    {
        stack.emplace_back(std::forward<Args>(args)...);
    }

    inline Variable pop()
    {
        if(stack.empty())
            throw SemanticError("Stack empty!");
        Variable var = stack.back();
        stack.pop_back();
        return var;
    }

    bool finalised;
    Statement *stmt;
    Table *table;
    std::vector<Variable> stack;
};


#endif //TESTDB_QUERYVM_H
