#include <iostream>
#include <Parser.h>
#include <QueryVM.h>

int main()
{
    while(true)
    {
        std::string query = "SELECT a FROM tab WHERE b == \"gav\";";
        std::cout << "Query: ";
        std::getline(std::cin, query);
        auto lexer = std::make_shared<Lexer>();
        lexer->lex(query);
        Parser parser(lexer);
        Statement stmt;
        parser.parse(&stmt);

        QueryVM vm;
        vm.eval_stmt(&stmt);
        row_t row;
        while(vm.fetch_row(&row))
        {
            for(auto &r : row)
            {
                if(r.type == Variable::Type::INT)
                    std::cout << r.store.int64 << ", ";
                else if(r.type == Variable::Type::STRING)
                    std::cout << std::string(r.store.str, r.store.len) << ", ";
            }
            std::cout << "\n";
        }
        vm = {};
    }

    return 0;
}
