#include <iostream>
#include <functional>
#include <Parser.h>
#include <exceptions/SemanticError.h>
#include <Opcode.h>
#include <Table.h>

Parser::Parser(std::shared_ptr<StorageEngine> storage_engine_, std::shared_ptr<Lexer> lexer_)
: storage_engine(std::move(storage_engine_)),
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
    lexer->legal_lookahead(Lexer::Token::SELECT, Lexer::Token::INSERT, Lexer::Token::SHOW, Lexer::Token::DESC);
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
        default:
            throw SemanticError("Unknown query type '" + lexer->current().data + "'");
    }
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
        lexer->advance();
        table_name(stmt);
    }

    if(lexer->match(Lexer::Token::WHERE))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_where_clause);
        cap_stmt(stmt->compiled_where_clause);
    }

    if(lexer->match(Lexer::Token::LIMIT))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_limit_clause);
        cap_stmt(stmt->compiled_limit_clause);
    }

    //Fill in referenced columns now the table name is known
    for(size_t a = 0; a < stmt->accessed_columns.size(); a++)
    {
        auto table = storage_engine->load_table(stmt->table_name);
        auto index = table->get_column_index(stmt->accessed_columns[a]);
        if(!index)
            throw SemanticError("No such column '" + stmt->accessed_columns[a] + "'");

        stmt->column_redirect[a] = *index;
    }
}

void Parser::insert_query(Statement *stmt)
{
    //Evaluate
    stmt->query_type = Lexer::Token::INSERT;
    lexer->legal_lookahead(Lexer::Token::INTO);
    lexer->advance();
    table_name(stmt);
    lexer->legal_lookahead(Lexer::Token::VALUES, Lexer::Token::SELECT);
    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, stmt->compiled_insert_values_clause);
        cap_stmt(stmt->compiled_insert_values_clause);
        return;
    }

    lexer->advance();
    lexer->legal_lookahead(Lexer::Token::OPEN_PARENTHESIS);
    lexer->advance();

    expr(stmt, stmt->compiled_insert_values_clause);
    while(lexer->match(Lexer::Token::COMMA))
    {
        lexer->advance();
        expr(stmt, stmt->compiled_insert_values_clause);
    }
    cap_stmt(stmt->compiled_insert_values_clause);

    lexer->legal_lookahead(Lexer::Token::CLOSE_PARENTHESIS);
    lexer->advance();
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
    stmt->query_type = lexer->current().type;
    lexer->legal_lookahead(Lexer::Token::TABLE);
    lexer->advance();

    //Extract table name
    table_name(stmt);

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

void Parser::table_name(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::ID);
    stmt->table_name = lexer->current().data;
    lexer->advance();
}

void Parser::column_definition(Statement *stmt)
{

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
    while(lexer->match(Lexer::Token::DOES_EQUAL, Lexer::Token::DOES_NOT_EQUAL))
    {

        if(lexer->match(Lexer::Token::DOES_EQUAL)) // ==
        {
            lexer->advance();
            expr_l3(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_EQ);
            continue;
        }
        if(lexer->match(Lexer::Token::DOES_NOT_EQUAL)) // !=
        {
            lexer->advance();
            expr_l3(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_NE);
            continue;
        }
    }
}

void Parser::expr_l2(Statement *stmt, std::string &output)
{
    expr_l3(stmt, output);
    expr_l2_(stmt, output);
}

void Parser::expr_l2_(Statement *stmt, std::string &output)
{
    while(lexer->match(Lexer::Token::ANGULAR_OPEN, Lexer::Token::ANGULAR_CLOSE))
    {
        if(lexer->match(Lexer::Token::ANGULAR_OPEN)) // <
        {
            lexer->advance();
            expr_l3(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_LT);
            continue;
        }

        if(lexer->match(Lexer::Token::ANGULAR_CLOSE)) // >
        {
            lexer->advance();
            expr_l3(stmt, output);
            output.append(sizeof(uint8_t), (char)Opcode::COMP_GT);
            continue;
        }
    }
}

void Parser::expr_l3(Statement *stmt, std::string &output)
{
    term(stmt, output);
    expr_l3_(stmt, output);
}

void Parser::expr_l3_(Statement *stmt, std::string &output)
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
    if(!lexer->legal_lookahead(Lexer::Token::STRING, Lexer::Token::INT, Lexer::Token::ID, Lexer::Token::SELECT))
    {
        return;
    }

    if(lexer->match(Lexer::Token::INT))
    {
        output.append(sizeof(uint8_t), (char)Opcode::PUSH_INT64);
        int64_t num = std::stoll(lexer->current().data);
        char *arr = (char*)&num;
        output.append(arr, sizeof(int64_t*));
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::STRING))
    {
        output.append(sizeof(uint8_t), (char)Opcode::PUSH_STRING);
        output.append(sizeof(uint8_t), (char)stmt->strings.size());
        stmt->strings.emplace_back(lexer->current().data);
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::ID))
    {
        output.append(sizeof(uint8_t), (char)Opcode::LOAD_COL);
        output.append(sizeof(uint8_t), (char)stmt->column_redirect.size());
        stmt->accessed_columns.emplace_back(lexer->current().data);
        stmt->column_redirect.emplace_back(0);
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, output);
    }
}

void Parser::cap_stmt(std::string &stmt)
{
    if(stmt.empty())
        return;
    stmt.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
}

void Parser::subselect(Statement *stmt, std::string &output)
{
    output.append(sizeof(uint8_t), (char)Opcode::EXEC_SUBQUERY);
    output.append(sizeof(uint8_t), (char)stmt->nested_statements.size());
    stmt->nested_statements.emplace_back();
    select_query(&stmt->nested_statements.back());
}
