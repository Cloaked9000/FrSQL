#ifndef FRSQL_FRSQL_H
#define FRSQL_FRSQL_H

#include <Database.h>
#include <QueryVM.h>
#include <Parser.h>
#include <functional>

class Frsql
{
public:
    explicit Frsql(std::unique_ptr<Filesystem> filesystem)
    {
        database = std::make_shared<Database>(std::move(filesystem));
        vm = std::make_unique<QueryVM>(database);
        lexer = std::make_shared<Lexer>();
        parser = std::make_unique<Parser>(database, lexer);
    }

    ~Frsql()
    {
        vm = nullptr;
        lexer = nullptr;
        parser = nullptr;
        database = nullptr;
    }

    int exec(const std::string_view sql)
    {
        return exec(sql, [](const row_t&){});
    }

	template<typename T>
	int exec(const std::string_view sql, const T &callback)
	{
        stmt.reset();
        vm->reset();
        lexer->lex(sql);
        parser->parse(&stmt);
        vm->eval_stmt(&stmt);

        row_t row;
        while (vm->fetch_row(&row))
        {
            callback(row);
        }
        return 0;
	}

private:
    Statement stmt;

    std::unique_ptr<QueryVM> vm;
    std::unique_ptr<Parser> parser;
    std::shared_ptr<Lexer> lexer;
    std::shared_ptr<Database> database;
};

#endif // FRSQL_FRSQL_H