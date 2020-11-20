#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <stack>
#include <any>
#include <utility>
#include <exceptions/SemanticError.h>
#include <cassert>
#include "Lexer.h"
#include "Variable.h"
#include "Statement.h"
#include "StorageEngine.h"

static int tmp_varname = 0;

class Parser
{
public:

    void operator=(const Parser &) = delete;
    void operator=(Parser &&) = delete;
    Parser(const Parser &) = delete;
    Parser(Parser &&) = delete;


    Parser(std::shared_ptr<StorageEngine> storage_engine, std::shared_ptr<Lexer> lexer);


    void parse(Statement *stmt);

private:

    //Parser
    void query(Statement *stmt);
    void select_query(Statement *stmt);
    void insert_query(Statement *stmt);
    void show_query(Statement *stmt);
    void desc_query(Statement *stmt);
    void create_query(Statement *stmt);
    void table_name(Statement *stmt);
    void result_column(Statement *stmt);
    void column_definition(Statement *stmt);

    void expr(Statement *stmt, std::string &output);
    void expr_(Statement *stmt, std::string &output);
    void expr_l2(Statement *stmt, std::string &output);
    void expr_l2_(Statement *stmt, std::string &output);
    void expr_l3(Statement *stmt, std::string &output);
    void expr_l3_(Statement *stmt, std::string &output);
    void term(Statement *stmt, std::string &output);
    void term_(Statement *stmt, std::string &output);
    void type(Statement *stmt, std::string &output);
    void factor(Statement *stmt, std::string &output);
    void subselect(Statement *stmt, std::string &output);
    
    //State


    //Misc
    void cap_stmt(std::string &stmt);

    //Dependencies
    std::shared_ptr<StorageEngine> storage_engine;
    std::shared_ptr<Lexer> lexer;

};


#endif //COMPILER_PARSER_H
