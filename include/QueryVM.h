//
// Created by fred.nicolson on 21/02/2020.
//

#ifndef TESTDB_QUERYVM_H
#define TESTDB_QUERYVM_H

#include <utility>
#include <vector>
#include <memory>
#include "Variable.h"

class Table;
class Statement;
class Database;
typedef std::vector<Variable> row_t;
class QueryVM
{
public:
    QueryVM(std::shared_ptr<Database> database);
    bool fetch_row(row_t *row);
    void eval_stmt(Statement *stmt);
    void reset();

private:
    bool run_cycle();
    bool run_select_cycle();
    bool run_insert_cycle();
    bool run_show_cycle();
    bool run_desc_cycle();
    bool run_create_cycle();
    bool run_delete_cycle();
    bool run_update_cycle();

    size_t exec(Statement *stmt, std::string_view bytecode);

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

    inline void push_state()
    {
        state_stack.emplace_back(state);
    }

    inline void pop_state()
    {
        if(state_stack.empty())
            throw SemanticError("State stack empty!");
        state = state_stack.back();
        state_stack.pop_back();
    }

    struct State
    {
        State()
        {
            reset();
        }

        void reset()
        {
            finalised = false;
            stmt = nullptr;
            table = nullptr;
            row = 0;
        }

        bool finalised;
        Statement *stmt;
        std::shared_ptr<Table> table;
        size_t row;
        std::vector<size_t> frames;
    };

    // State
    State state;
    std::vector<Variable> stack;
    std::vector<State> state_stack;

    // Dependencies
    std::shared_ptr<Database> database;
};


#endif //TESTDB_QUERYVM_H
