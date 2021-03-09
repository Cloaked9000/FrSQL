#ifndef FRSQL_FRSQL_H
#define FRSQL_FRSQL_H

#include <Database.h>
#include <QueryVM.h>
#include <Parser.h>

class Frsql
{
public:
    Frsql()
    {
        database = std::make_shared<Database>();
        vm = std::make_unique<QueryVM>(database);
    }

	template<typename T>
	int exec(const std::string_view sql, const T &callback)
	{
        auto lexer = std::make_shared<Lexer>();
        lexer->lex(sql);

        Parser parser(database, lexer);
        Statement stmt;
        parser.parse(&stmt);

        vm->eval_stmt(&stmt);

        row_t row;
        while (vm->fetch_row(&row))
        {
            callback(row);
        }
        
        vm->reset();
        return 0;
	}

private:
    std::unique_ptr<QueryVM> vm;
    std::shared_ptr<Database> database;
};

#endif // FRSQL_FRSQL_H