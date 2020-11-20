#include <iostream>
#include <functional>
#include <Parser.h>
#include <exceptions/SemanticError.h>
#include <Opcode.h>

Parser::Parser(std::shared_ptr<Lexer> lexer_)
: lexer(std::move(lexer_))
{

}

void Parser::parse(Statement *stmt)
{
    query(stmt);
}

void Parser::query(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::SELECT);
    stmt->query_type = lexer->current().type;
    switch(lexer->current().type)
    {
        case Lexer::Token::SELECT:
            lexer->advance();
            select_query(stmt);
            break;
        default:
            throw SemanticError("Unknown query type '" + lexer->current().data + "'");
    }
}

void Parser::select_query(Statement *stmt)
{
    result_column(stmt);
    lexer->legal_lookahead(Lexer::Token::FROM);
    lexer->advance();
    table_name(stmt);
    lexer->legal_lookahead(Lexer::Token::WHERE);
    lexer->advance();
    expr(stmt, stmt->compiled_where_clause);
    stmt->compiled_where_clause.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
}

void Parser::table_name(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::ID);
    stmt->table_name = lexer->current().data;
    lexer->advance();
}

void Parser::result_column(Statement *stmt)
{
    std::string clause;
    expr(stmt, clause);
    clause.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
    stmt->compiled_result_clauses.emplace_back(std::move(clause));
    while(lexer->match(Lexer::Token::COMMA))
    {
        clause.clear();
        lexer->advance();
        expr(stmt, clause);
        clause.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
        stmt->compiled_result_clauses.emplace_back(clause);
    }
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
    if(!lexer->legal_lookahead(Lexer::Token::STRING, Lexer::Token::INT, Lexer::Token::ID))
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
        output.append(sizeof(uint8_t), (char)stmt->strings.size());
        stmt->strings.emplace_back(lexer->current().data);
        lexer->advance();
        return;
    }
}