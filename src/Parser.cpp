#include <iostream>
#include <functional>
#include <algorithm>
#include <Parser.h>
#include <exceptions/SemanticError.h>
#include <Opcode.h>
#include "table/Table.h"
#include <charconv>

Parser::Parser(std::shared_ptr<Database> database_, std::shared_ptr<Lexer> lexer_)
: database(std::move(database_)),
  lexer(std::move(lexer_))
{

}

void Parser::parse(Statement *stmt)
{
    //Evaluate query
    query(stmt);
}

void Parser::query(Statement *stmt)
{
    switch(lexer->current().type)
    {
        case Lexer::Token::SELECT:
            lexer->advance();
            select_query(stmt);
            break;
        case Lexer::Token::INSERT:
            lexer->advance();
            insert_query(stmt);
            break;
        case Lexer::Token::SHOW:
            lexer->advance();
            show_query(stmt);
            break;
        case Lexer::Token::DESC:
            lexer->advance();
            desc_query(stmt);
            break;
        case Lexer::Token::CREATE:
            lexer->advance();
            create_query(stmt);
            break;
        case Lexer::Token::DELETE:
            lexer->advance();
            delete_query(stmt);
            break;
        case Lexer::Token::UPDATE:
            lexer->advance();
            update_query(stmt);
            break;
        default:
            //Print suitable error if unknown type
            lexer->legal_lookahead(Lexer::Token::SELECT, Lexer::Token::INSERT, Lexer::Token::SHOW, Lexer::Token::DESC, Lexer::Token::CREATE, Lexer::Token::DELETE, Lexer::Token::UPDATE);
    }

    //Fill in referenced columns now the table name is known
    link_stmt(stmt);
}

void Parser::select_query(Statement *stmt)
{
    //Evaluate
    stmt->query_type = Lexer::Token::SELECT;
    result_column(stmt);
    while(lexer->match(Lexer::Token::COMMA))
    {
        lexer->advance();
        result_column(stmt);
    }
    cap_stmt(stmt->compiled_result_clauses);

    if(lexer->match(Lexer::Token::FROM))
    {
        do
        {
            lexer->advance();
            table_or_subquery(stmt);
        } while(lexer->match(Lexer::Token::COMMA));
    }

    if(lexer->match(Lexer::Token::WHERE))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_where_clause);
        cap_stmt(stmt->compiled_where_clause);
    }

    if(lexer->match(Lexer::Token::ORDER))
    {
        lexer->advance();
        lexer->legal_lookahead(Lexer::Token::BY);

        do
        {
            lexer->advance(); //skips over both BY *or* commas if looped
            ordering_term(stmt);
        } while(lexer->match(Lexer::Token::COMMA));
    }

    if(lexer->match(Lexer::Token::LIMIT))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_limit_clause);
        cap_stmt(stmt->compiled_limit_clause);
    }
}

void Parser::insert_query(Statement *stmt)
{
    //INSERT INTO x VALUES/SELECT
    stmt->query_type = Lexer::Token::INSERT;
    lexer->legal_lookahead(Lexer::Token::INTO);
    lexer->advance();
    table_name(stmt);

    //Support multiple inserts, (col1, col2, ...)
    if(lexer->match(Lexer::Token::OPEN_PARENTHESIS))
    {
        lexer->advance();
        column_name(stmt);
        while(lexer->match(Lexer::Token::COMMA))
        {
            lexer->advance();
            column_name(stmt);
        }

        lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
        lexer->advance();
    }

    //SELECT ...
    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, stmt->compiled_insert_values_clause);
        cap_stmt(stmt->compiled_insert_values_clause);
        return;
    }

    //VALUES (..., ..., ...), ...
    if(lexer->match(Lexer::Token::VALUES))
    {
        // todo validate number of values
        do
        {
            lexer->advance();
            lexer->legal_lookahead(Lexer::Token::OPEN_PARENTHESIS);
            lexer->advance();

            expr(stmt, stmt->compiled_insert_values_clause);
            while(lexer->match(Lexer::Token::COMMA))
            {
                lexer->advance();
                expr(stmt, stmt->compiled_insert_values_clause);
            }

            lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
            lexer->advance();
        } while(lexer->match(Lexer::Token::COMMA));

        cap_stmt(stmt->compiled_insert_values_clause);
        return;
    }

    //If it was neither, print appropriate error
    lexer->legal_lookahead(Lexer::Token::VALUES, Lexer::Token::SELECT);
}

void Parser::update_query(Statement *stmt)
{
    stmt->query_type = Lexer::Token::UPDATE;
    table_name(stmt);
    lexer->legal_lookahead(Lexer::Token::SET);
    lexer->advance();

    column_name(stmt);
    lexer->legal_lookahead(Lexer::Token::EQUALS);
    lexer->advance();
    expr(stmt, stmt->compiled_update_clause);
    while(lexer->match(Lexer::Token::COMMA))
    {
        lexer->advance();
        column_name(stmt);
        lexer->legal_lookahead(Lexer::Token::EQUALS);
        lexer->advance();
        expr(stmt, stmt->compiled_update_clause);
    }
    cap_stmt(stmt->compiled_update_clause);

    if(lexer->match(Lexer::Token::WHERE))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_where_clause);
        cap_stmt(stmt->compiled_where_clause);
    }
}

void Parser::column_name(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::ID);
    auto table = database->load_table(stmt->table_id);
    auto col_id = table->get_metadata().get_column_index(lexer->current().data);
    if(col_id == ID_NONE)
    {
        throw SyntaxError("No such column '" + std::string(lexer->current().data) + "'!");
    }
    stmt->column_ids.emplace_back(col_id);
    lexer->advance();
}

void Parser::ordering_term(Statement *stmt)
{
    //note: Default order value is ascending (already set)
    expr(stmt, stmt->compiled_ordering_clause);
    cap_stmt(stmt->compiled_ordering_clause);
    if(lexer->match(Lexer::Token::ASC))
    {
        lexer->advance();
    }

    if(lexer->match(Lexer::Token::DESC))
    {
        lexer->advance();
        stmt->order = Statement::Order::Descending;
    }
}

void Parser::show_query(Statement *stmt)
{
    stmt->query_type = Lexer::Token::SHOW;
    lexer->legal_lookahead(Lexer::Token::TABLES);
    lexer->advance();

}

void Parser::desc_query(Statement *stmt)
{
    stmt->query_type = Lexer::Token::DESC;
    lexer->legal_lookahead(Lexer::Token::ID);
    table_name(stmt);
}

void Parser::create_query(Statement *stmt)
{
    //Skip over 'TABLE'
    stmt->query_type = Lexer::Token::CREATE;
    lexer->legal_lookahead(Lexer::Token::TABLE);
    lexer->advance();

    //Extract table name
    lexer->legal_lookahead(Lexer::Token::ID);
    stmt->new_table_name = lexer->current().data;
    lexer->advance();

    //Iterate through columns in new table
    lexer->legal_lookahead(Lexer::Token::OPEN_PARENTHESIS);
    lexer->advance();

    column_definition(stmt);
    while(lexer->match(Lexer::Token::COMMA))
    {
        lexer->advance();
        column_definition(stmt);
    }

    lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
    lexer->advance();
}

void Parser::delete_query(Statement* stmt)
{
    //Skip over FROM
    stmt->query_type = Lexer::Token::DELETE;
    lexer->legal_lookahead(Lexer::Token::FROM);
    lexer->advance();

    //Table name
    table_name(stmt);

    //Where statement, if any (optional)
    if (lexer->match(Lexer::Token::WHERE))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_where_clause);
        cap_stmt(stmt->compiled_where_clause);
    }
}

void Parser::column_definition(Statement* stmt)
{
    ColumnMetadata metadata;
    lexer->legal_lookahead(Lexer::Token::ID);
    metadata.name = lexer->current().data;
    metadata.id = stmt->column_definitions.size();
    lexer->advance();

    lexer->legal_lookahead(Lexer::Token::ID);
    std::string upper;
    std::transform(lexer->current().data.begin(), lexer->current().data.end(), std::back_inserter(upper), ::toupper);
    metadata.type = upper == "INT" ? Variable::Type::INT : Variable::Type::STRING; //todo: don't hardcode like this
    lexer->advance();
    stmt->column_definitions.emplace_back(std::move(metadata));
}


void Parser::table_name(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::ID);
    if(auto id = database->lookup_table(lexer->current().data))
    {
        stmt->table_id = *id;
    }
    else
    {
        throw SemanticError("No such table '" + std::string(lexer->current().data) + "'");
    }

    lexer->advance();
}

void Parser::table_or_subquery(Statement *stmt)
{
    if(lexer->match(Lexer::Token::ID)) //is table
    {
        table_name(stmt);
        return;
    }

    if(lexer->match(Lexer::Token::OPEN_PARENTHESIS)) //is subquery
    {
        lexer->advance();
        subselect(stmt, stmt->compiled_from_clause);
        lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
        lexer->advance();
        return;
    }

    lexer->legal_lookahead(Lexer::Token::ID, Lexer::Token::OPEN_PARENTHESIS);
}

void Parser::result_column(Statement *stmt)
{
    if(lexer->match(Lexer::Token::ASTERISK)) // *
    {
        stmt->compiled_result_clauses.append(sizeof(uint8_t), (char)Opcode::LOAD_ALL);
        lexer->advance();
        return;
    }

    expr(stmt, stmt->compiled_result_clauses); //expr
}

void Parser::expr(Statement *stmt, std::string &output)
{
    expr_l2(stmt, output);
    expr_(stmt, output);
}

void Parser::expr_(Statement *stmt, std::string &output)
{
    bool should_invert = false;
    if(lexer->match(Lexer::Token::NOT))
    {
        should_invert = true;
        lexer->advance();
        lexer->legal_lookahead(Lexer::Token::IN);
    }

    if(lexer->match(Lexer::Token::IN))
    {
        lexer->advance();
        in(stmt, output);
    }

    if(should_invert)
    {
        output.append(sizeof(uint8_t), (char)Opcode::FLIP);
    }
}

void Parser::expr_l2(Statement *stmt, std::string &output)
{
    bool should_invert = false;
    if(lexer->match(Lexer::Token::NOT))
    {
        should_invert = true;
        lexer->advance();
    }

    expr_l3(stmt, output);
    expr_l2_(stmt, output);

    if(should_invert)
    {
        output.append(sizeof(uint8_t), (char)Opcode::FLIP);
    }
}

void Parser::expr_l2_(Statement *stmt, std::string &output)
{
    while(lexer->match(Lexer::Token::EQUALS, Lexer::Token::DOES_NOT_EQUAL))
    {

        if(lexer->match(Lexer::Token::EQUALS)) // ==
        {
            lexer->advance();
            expr_l4(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_EQ);
            continue;
        }
        if(lexer->match(Lexer::Token::DOES_NOT_EQUAL)) // !=
        {
            lexer->advance();
            expr_l4(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_NE);
            continue;
        }
    }
}

void Parser::in(Statement *stmt, std::string &output)
{
    lexer->legal_lookahead(Lexer::Token::OPEN_PARENTHESIS);
    lexer->advance();
    output.append(sizeof(uint8_t), (char)Opcode::FRAME_MARKER);

    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, output);
    }
    else
    {
        expr(stmt, output);
        while(lexer->match(Lexer::Token::COMMA))
        {
            lexer->advance();
            expr(stmt, output);
        }
    }

    lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
    lexer->advance();

    output.append(sizeof(uint8_t), (char)Opcode::FILTER_MUTUAL);
}

void Parser::expr_l3(Statement *stmt, std::string &output)
{
    expr_l4(stmt, output);
    expr_l3_(stmt, output);
}

void Parser::expr_l3_(Statement *stmt, std::string &output)
{
    while(lexer->match(Lexer::Token::ANGULAR_OPEN, Lexer::Token::ANGULAR_CLOSE))
    {
        if(lexer->match(Lexer::Token::ANGULAR_OPEN)) // <
        {
            lexer->advance();
            expr_l4(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_LT);
            continue;
        }

        if(lexer->match(Lexer::Token::ANGULAR_CLOSE)) // >
        {
            lexer->advance();
            expr_l4(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_GT);
            continue;
        }
    }
}

void Parser::expr_l4(Statement *stmt, std::string &output)
{
    term(stmt, output);
    expr_l4_(stmt, output);
}

void Parser::expr_l4_(Statement *stmt, std::string &output)
{
    while(lexer->match(Lexer::Token::PLUS, Lexer::Token::MINUS))
    {
        if(lexer->match(Lexer::Token::PLUS))
        {
            lexer->advance();
            term(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::ADD);
            continue;
        }

        if(lexer->match(Lexer::Token::MINUS))
        {
            lexer->advance();
            term(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::SUB);
            continue;
        }
    }
}

void Parser::term(Statement *stmt, std::string &output)
{
    factor(stmt, output);
    term_(stmt, output);
}

void Parser::term_(Statement *stmt, std::string &output)
{
    while(lexer->match(Lexer::Token::ASTERISK, Lexer::Token::FORWARDS_SLASH, Lexer::Token::MODULO))
    {
        if(lexer->match(Lexer::Token::ASTERISK))
        {
            lexer->advance();
            factor(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::MULT);
            continue;
        }

        if(lexer->match(Lexer::Token::FORWARDS_SLASH))
        {
            lexer->advance();
            factor(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::DIV);
            continue;
        }

        if(lexer->match(Lexer::Token::MODULO))
        {
            lexer->advance();
            factor(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::MOD);
            continue;
        }
    }
}

void Parser::factor(Statement *stmt, std::string &output)
{
    if(lexer->match(Lexer::Token::OPEN_PARENTHESIS))
    {
        lexer->advance();
        expr(stmt, output);
        if(!lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS))
        {
            return;
        }
        lexer->advance();
        return;
    }

    type(stmt, output);
}

void Parser::type(Statement *stmt, std::string &output)
{
    if(lexer->match(Lexer::Token::INT))
    {
        int64_t num = 0;
        const std::string_view data = lexer->current().data;
        std::from_chars(data.data(), data.data() + data.size(), num);
        output.append(sizeof(uint8_t), (char)Opcode::PUSH_INT64);
        output.append(reinterpret_cast<char*>(&num), sizeof(int64_t));
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::STRING))
    {
        output.append(sizeof(uint8_t), (char)Opcode::PUSH_STRING);
        output.append(sizeof(uint8_t), (char)stmt->strings.size()); // push index of string within statement's string table
        stmt->strings.emplace_back(lexer->current().data);
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::ID))
    {
        output.append(sizeof(uint8_t), (char)Opcode::LOAD_COL);
        output.append(sizeof(uint8_t), '\0'); //placeholder to be linked
        stmt->accessed_columns.emplace_back(Statement::Link((uint8_t*)&output.back(), lexer->current().data));
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, output);
        return;
    }

    //Nothing matched, print suitable error
    lexer->legal_lookahead(Lexer::Token::STRING, Lexer::Token::INT, Lexer::Token::ID, Lexer::Token::SELECT);
}

void Parser::subselect(Statement *stmt, std::string &output)
{
    output.append(sizeof(uint8_t), (char)Opcode::EXEC_SUBQUERY);
    output.append(sizeof(uint8_t), (char)stmt->nested_statements.size());
    stmt->nested_statements.emplace_back();
    Statement *nested = &stmt->nested_statements.back();
    select_query(nested);
    link_stmt(nested);
}

void Parser::cap_stmt(std::string &stmt)
{
    if(stmt.empty())
        return;
    stmt.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
}

void Parser::link_stmt(Statement *stmt)
{
    if (stmt->table_id == ID_NONE)
    {
        return;
    }

    for (size_t a = 0; a < stmt->accessed_columns.size(); a++)
    {
        auto table = database->load_table(stmt->table_id);
        auto index = table->get_metadata().get_column_index(stmt->accessed_columns[a].column_name);
        if (index == ID_NONE)
        {
            throw SemanticError("No such column '" + std::string(stmt->accessed_columns[a].column_name) + "'");
        }

        assert(index <= std::numeric_limits<uint8_t>::max()); //todo: increase max size
        *stmt->accessed_columns[a].bytecode_pos = static_cast<uint8_t>(index);
    }
}