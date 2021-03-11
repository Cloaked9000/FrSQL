#include <iostream>
#include <functional>
#include <Parser.h>
#include <exceptions/SemanticError.h>
#include <Opcode.h>
#include <Table.h>
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
    lexer->legal_lookahead(Lexer::Token::SELECT, Lexer::Token::INSERT, Lexer::Token::SHOW, Lexer::Token::DESC, Lexer::Token::CREATE, Lexer::Token::DELETE);
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
        default:
            throw SemanticError("Unknown query type '" + std::string(lexer->current().data) + "'");
    }

    //Fill in referenced columns now the table name is known
    resolve_table_references(stmt);
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
    lexer->advance();

    lexer->legal_lookahead(Lexer::Token::ID, Lexer::Token::INT, Lexer::Token::STRING);
    metadata.type = lexer->current().data;
    lexer->advance();
    stmt->column_definitions.emplace_back(std::move(metadata));
}


void Parser::table_name(Statement *stmt)
{
    lexer->legal_lookahead(Lexer::Token::ID);
    stmt->table_id = database->lookup_table(lexer->current().data);
    lexer->advance();
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
    in(stmt, output);
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

void Parser::in(Statement *stmt, std::string &output)
{
    if(lexer->match(Lexer::Token::IN))
    {
        lexer->advance();
        lexer->legal_lookahead(Lexer::Token::OPEN_PARENTHESIS);
        lexer->advance();
        output.append(sizeof(uint8_t), (char)Opcode::FRAME_BEGIN);

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
        int64_t num = 0;
        std::from_chars(lexer->current().data.data(), lexer->current().data.data() + lexer->current().data.size(), num);
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
        output.append(sizeof(uint8_t), (char)stmt->column_redirect.size());
        stmt->accessed_columns.emplace_back(lexer->current().data);
        stmt->column_redirect.emplace_back(std::numeric_limits<size_t>::max());
        lexer->advance();
        return;
    }

    if(lexer->match(Lexer::Token::SELECT))
    {
        lexer->advance();
        subselect(stmt, output);
    }
}

void Parser::subselect(Statement *stmt, std::string &output)
{
    output.append(sizeof(uint8_t), (char)Opcode::EXEC_SUBQUERY);
    output.append(sizeof(uint8_t), (char)stmt->nested_statements.size());
    stmt->nested_statements.emplace_back();
    Statement *nested = &stmt->nested_statements.back();
    select_query(nested);
    resolve_table_references(nested);
}

void Parser::cap_stmt(std::string &stmt)
{
    if(stmt.empty())
        return;
    stmt.append(sizeof(uint8_t), (uint8_t)Opcode::QUIT);
}

void Parser::resolve_table_references(Statement *stmt)
{
    if (stmt->table_id == ID_NONE)
    {
        return;
    }

    for (size_t a = 0; a < stmt->accessed_columns.size(); a++)
    {
        auto table = database->load_table(stmt->table_id);
        auto index = table->get_column_index(stmt->accessed_columns[a]);
        if (!index)
        {
            throw SemanticError("No such column '" + stmt->accessed_columns[a] + "'");
        }

        stmt->column_redirect[a] = *index;
    }
}