//
// Created by fred.nicolson on 21/02/2020.
//

#include <iostream>
#include <exceptions/SemanticError.h>
#include <Lexer.h>
#include "QueryVM.h"
#include "Statement.h"

void QueryVM::eval_stmt(Statement *stmt_)
{
    stmt = stmt_;
    switch(stmt->query_type)
    {
        case Lexer::Token::Type::SELECT:
            eval_select(stmt);
            break;
        default:
            throw SemanticError("Can't evaluate unknown query type!");
    }
}
void QueryVM::eval_select(Statement *stmt)
{
    static Table tab;
    tab = Table();
    finalised = false;
    table = &tab;
}

bool QueryVM::fetch_row(row_t *row)
{
    while(!finalised)
    {
        exec(stmt, stmt->compiled_where_clause);
        auto where_matched = pop(); //pop match result from stack
        if(where_matched.store.int64 == 0)
        {
            finalised = !table->advance();
            continue;
        }

        row->resize(stmt->compiled_result_clauses.size());
        for(size_t a = 0; a < stmt->compiled_result_clauses.size(); ++a)
        {
            exec(stmt, stmt->compiled_result_clauses[a]);
            (*row)[a] = pop();
        }

        finalised = !table->advance();
        return true;
    }
    return false;
}

//std::cout << "Opcode: " << (int)bytecode[off] << ". " << std::endl;
#define DISPATCH() goto *dispatch_table[bytecode[off]]
#define CONSUME_BYTES(x) off += x;
void QueryVM::exec(Statement *stmt, std::string_view bytecode)
{
    size_t off = 0;
    static void *dispatch_table[] = {&&do_push_int64, &&do_push_string, &&do_mult, &&do_div, &&do_mod, &&do_sub, &&do_add, &&do_comp_ne, &&do_comp_eq, &&do_comp_gt, &&do_comp_lt, &&do_load_col,  &&do_exit};
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
            std::string &str = stmt->strings[strings_off];
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
            const auto strings_off = (uint8_t)bytecode[off + 1];
            auto col = table->load_col(stmt->strings[strings_off]);
            push(col);
            CONSUME_BYTES(2);
            DISPATCH();
        }
        do_exit:
            return;
    }
}