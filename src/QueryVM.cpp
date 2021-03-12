//
// Created by fred.nicolson on 21/02/2020.
//

#include <iostream>
#include <exceptions/SemanticError.h>
#include <Lexer.h>
#include <string>
#include <string_view>
#include <stdint.h>
#include <span>
#include "QueryVM.h"
#include "Statement.h"
#include "Database.h"
#include "Table.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
#ifndef _UNISTD_H
#define _UNISTD_H
#ifdef _WIN64
#define ssize_t __int64
#else
#define ssize_t long
#endif
#endif

QueryVM::QueryVM(std::shared_ptr<Database> database)
: database(std::move(database))
{

}

void QueryVM::eval_stmt(Statement *stmt_)
{
    state.reset();
    state.stmt = stmt_;

    //Load in table if specified
    if (state.stmt->table_id != ID_NONE)
    {
        state.table = database->load_table(state.stmt->table_id);
    }

    //Evaluate any LIMIT clauses
    if (!state.stmt->compiled_limit_clause.empty())
    {
        exec(state.stmt, state.stmt->compiled_limit_clause);
        Variable var = pop();
        if (var.type != Variable::Type::INT)
        {
            throw SemanticError("Limit must be integral");
        }
        state.stmt->evaluated_limit = var.store.int64;
    }
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
        case Lexer::Token::Type::CREATE:
            return run_create_cycle();
        case Lexer::Token::Type::DELETE:
            return run_delete_cycle();
        default:
            break;
    }
    abort();
}

bool QueryVM::run_delete_cycle()
{
    //Delete operations complete within a single cycle always
    state.finalised = true;

    //If there's no where clause, just delete everything in the table
    if (state.stmt->compiled_where_clause.empty())
    {
        auto table = database->load_table(state.stmt->table_id);
        table->clear();
        return true;
    }

    //Else we have a where clause, evaluate it against each row
    while (state.row < state.table->row_count())
    {
        exec(state.stmt, state.stmt->compiled_where_clause);
        auto where_matched = pop();
        if (where_matched.store.int64 > 0)
        {
            state.table->delete_row(state.row);
            continue;
        }
        state.row++;
    }

    return true;
}

bool QueryVM::run_create_cycle()
{
    database->create_table(state.stmt->new_table_name, state.stmt->column_definitions);
    return state.finalised = true;
}

bool QueryVM::run_select_cycle()
{
    if(!state.stmt->compiled_where_clause.empty())
    {
        exec(state.stmt, state.stmt->compiled_where_clause);
        auto where_matched = pop(); //pop match result from stack
        if(where_matched.store.int64 == 0)
        {
            state.finalised = state.table == nullptr || ++state.row >= state.table->row_count();
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

    state.finalised = state.table == nullptr || ++state.row >= state.table->row_count();
    return true;
}

bool QueryVM::run_insert_cycle()
{
    //Evaluate the insert values
    size_t value_count = exec(state.stmt, state.stmt->compiled_insert_values_clause);
    std::vector<Variable> ret;
    ret.resize(value_count);
    for(size_t a = value_count; a-- > 0;)
    {
        ret[a] = pop();
    }

    state.table->insert(std::move(ret));
    return state.finalised = true;
}

bool QueryVM::run_show_cycle()
{
    auto table_list = database->list_table_ids();
    for(const auto &id : table_list)
    {
        auto tab = database->load_table(id);
        const auto &meta = tab->get_metadata();
        push(meta.name.data(), meta.name.size());
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
#define CONSUME_BYTES(x) off += (x);
size_t QueryVM::exec(Statement *stmt, std::string_view bytecode)
{
    size_t before = stack.size();
    size_t off = 0;
    static void *dispatch_table[] = { &&do_exit, &&do_push_int64, &&do_push_string, &&do_mult, &&do_div, &&do_mod, &&do_sub, &&do_add, &&do_comp_ne, &&do_comp_eq, &&do_comp_gt, &&do_comp_lt, &&do_load_col, &&do_load_all, &&exec_sub_query, &&exec_frame_marker, &&exec_filter_mutual};
    DISPATCH();
    while(true)
    {
        do_exit:
        {
            break;
        }
        do_push_int64:
        {
            push(*(int64_t*)&bytecode[off + 1]);
            CONSUME_BYTES(sizeof(uint64_t) + 1);
            DISPATCH();
        }
        do_push_string:
        {
            const auto strings_off = (uint8_t)bytecode[off + 1];
            std::string_view str = state.stmt->strings[strings_off];
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
            const auto col_id = stmt->column_redirect.at(col_index);
            const auto &col = state.table->load_col(state.row, col_id);
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
            while(run_cycle());
            pop_state();

            CONSUME_BYTES(2);
            DISPATCH();
        };
        exec_frame_marker:
        {
            state.frames.emplace_back(stack.size());
            CONSUME_BYTES(1);
            DISPATCH();
        };
        exec_filter_mutual:
        {
            auto frame_pos = state.frames.back();
            state.frames.pop_back();

            bool match = std::find(stack.begin() + frame_pos, stack.end(), stack[frame_pos - 1]) != stack.end();
            stack.erase(stack.begin() + frame_pos - 1, stack.end());
            push(match);

            CONSUME_BYTES(1);
            DISPATCH();
        };
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

#pragma clang diagnostic pop