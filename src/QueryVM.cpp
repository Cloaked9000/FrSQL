//
// Created by fred.nicolson on 21/02/2020.
//

#include <iostream>
#include <exceptions/SemanticError.h>
#include <Lexer.h>
#include <string>
#include <string_view>
#include "QueryVM.h"
#include "Statement.h"
#include "StorageEngine.h"
#include "Table.h"

QueryVM::QueryVM(std::shared_ptr<StorageEngine> storage_engine)
: storage_engine(std::move(storage_engine))
{

}

void QueryVM::eval_stmt(Statement *stmt_)
{
    state.table = nullptr;
    state.finalised = false;
    state.stmt = stmt_;
    state.row = 0;
    switch(state.stmt->query_type)
    {
        case Lexer::Token::Type::SELECT:
            eval_select();
            break;
        case Lexer::Token::Type::INSERT:
            eval_insert();
            break;
        case Lexer::Token::Type::SHOW:
            eval_show();
            break;
        case Lexer::Token::Type::DESC:
            eval_desc();
            break;
        default:
            throw SemanticError("Can't evaluate unknown query type!");
    }
}
void QueryVM::eval_select()
{
    //Setup state - temp
    if(!state.stmt->table_name.empty())
    {
        state.table = storage_engine->load_table(state.stmt->table_name);
    }

    //Evaluate any LIMIT clauses
    if(!state.stmt->compiled_limit_clause.empty())
    {
        exec(state.stmt, state.stmt->compiled_limit_clause);
        Variable var = pop();
        if(var.type != Variable::Type::INT)
        {
            throw SemanticError("Limit must be integral");
        }
        state.stmt->evaluated_limit = var.store.int64;
    }
}

void QueryVM::eval_insert()
{
    //Setup state - temp
    if(!state.stmt->table_name.empty())
    {
        state.table = storage_engine->load_table(state.stmt->table_name);
    }
}

void QueryVM::eval_show()
{

}

void QueryVM::eval_desc()
{
    state.table = storage_engine->load_table(state.stmt->table_name);
}


bool QueryVM::run_cycle()
{
    if(state.finalised)
        return false;

    switch(state.stmt->query_type)
    {
        case Lexer::Token::Type::SELECT:
            return run_select_cycle();
        case Lexer::Token::Type::INSERT:
            return run_insert_cycle();
        case Lexer::Token::Type::SHOW:
            return run_show_cycle();
        case Lexer::Token::Type::DESC:
            return run_desc_cycle();
        default:
            break;
    }
    abort();
}

bool QueryVM::run_select_cycle()
{
    if(!state.stmt->compiled_where_clause.empty())
    {
        exec(state.stmt, state.stmt->compiled_where_clause);
        auto where_matched = pop(); //pop match result from stack
        if(where_matched.store.int64 == 0)
        {
            state.finalised = state.table == nullptr ? true : ++state.row >= state.table->row_count();
            return true;
        }
    }

    if(!state.stmt->compiled_result_clauses.empty())
    {
        state.stmt->rows_returned += exec(state.stmt, state.stmt->compiled_result_clauses);
        if(!state.stmt->compiled_limit_clause.empty() && state.stmt->rows_returned > state.stmt->evaluated_limit)
        {
            return state.finalised = true;
        }
    }

    state.finalised = state.table == nullptr ? true : ++state.row >= state.table->row_count();
    return true;
}

bool QueryVM::run_insert_cycle()
{
    //Evaluate the insert values
    size_t value_count = exec(state.stmt, state.stmt->compiled_insert_values_clause);
    std::vector<Variable> ret;
    ret.reserve(value_count);
    for(size_t a = 0; a < value_count; a++)
    {
        ret.emplace_back(pop());
    }

    state.table->insert(std::move(ret));
    return state.finalised = true;
}

bool QueryVM::run_show_cycle()
{
    auto table_list = storage_engine->list_tables();
    for(const auto &tab : table_list)
    {
        push(tab.data(), tab.size());
    }
    return state.finalised = true;
}

bool QueryVM::run_desc_cycle()
{
    std::string_view name = state.table->get_column_name(state.row);
    push(name.data(), name.size());
    state.finalised = ++state.row >= state.table->get_column_count();
    return true;
}

bool QueryVM::fetch_row(row_t *row)
{
    row->clear();
    size_t before = stack.size();
    bool ret = run_cycle();
    size_t diff = stack.size() - before;
    row->resize(diff);
    for(ssize_t b = 0; b < diff; b++)
    {
        (*row)[row->size() - b - 1] = pop();
    }
    return ret;
}

//std::cout << "Opcode: " << (int)bytecode[off] << ". " << std::endl;
#define DISPATCH() goto *dispatch_table[bytecode[off]]
#define CONSUME_BYTES(x) off += x;
size_t QueryVM::exec(Statement *stmt, std::string_view bytecode)
{
    size_t before = stack.size();
    size_t off = 0;
    static void *dispatch_table[] = {&&do_push_int64, &&do_push_string, &&do_mult, &&do_div, &&do_mod, &&do_sub, &&do_add, &&do_comp_ne, &&do_comp_eq, &&do_comp_gt, &&do_comp_lt, &&do_load_col, &&do_load_all, &&exec_sub_query, &&do_exit};
    DISPATCH();
    while(true)
    {
        do_push_int64:
        {
            push(*(int64_t*)&bytecode[off + 1]);
            CONSUME_BYTES(sizeof(uint64_t) + 1);
            DISPATCH();
        }
        do_push_string:
        {
            const auto strings_off = (uint8_t)bytecode[off + 1];
            std::string &str = state.stmt->strings[strings_off];
            push(str.data(), str.size());
            CONSUME_BYTES(2);
            DISPATCH();
        }
        do_mult:
        {
            push(pop() * pop());
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_div:
        {
            auto s1 = pop(), s2 = pop();
            push(s2 / s1);
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_mod:
        {
            auto s1 = pop(), s2 = pop();
            push(s2 % s1);
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_sub:
        {
            auto s1 = pop(), s2 = pop();
            push(s2 - s1);
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_add:
        {
            push(pop() + pop());
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_comp_ne:
        {
            push(pop() != pop());
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_comp_eq:
        {
            push(pop() == pop());
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_comp_gt:
        {
            auto s1 = pop(), s2 = pop();
            push(s2 > s1);
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_comp_lt:
        {
            auto s1 = pop(), s2 = pop();
            push(s2 < s1);
            CONSUME_BYTES(1);
            DISPATCH();
        }
        do_load_col:
        {
            const auto col_index = (uint8_t)bytecode[off + 1];
            auto col = state.table->load_col(state.row, stmt->column_redirect[col_index]);
            push(col);
            CONSUME_BYTES(2);
            DISPATCH();
        }
        do_load_all:
        {
            const auto &col = state.table->load_row(state.row);
            for(const auto &c : col)
            {
                push(c);
            }
            CONSUME_BYTES(1);
            DISPATCH();
        }
        exec_sub_query:
        {
            const auto query_off = (uint8_t)bytecode[off + 1];

            push_state();
            eval_stmt(&state.stmt->nested_statements[query_off]);
            run_cycle();
            pop_state();

            CONSUME_BYTES(2);
            DISPATCH();
        };
        do_exit:
            break;
    }

    if(before > stack.size())
        throw SemanticError("Query decreased stack size, bug!?");
    return stack.size() - before;
}

void QueryVM::reset()
{
    stack.clear();
    state_stack.clear();
}
