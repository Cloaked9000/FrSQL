#include <iostream>
#include <Parser.h>
#include <QueryVM.h>
#include <memory>
#include <string>
#include <chrono>
#include <StorageEngine.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
    auto storage = std::make_shared<StorageEngine>();
    while(true)
    {
        std::string query = "SELECT *, (select 5 * 5) FROM tab WHERE a == (SELECT a FROM tab WHERE b == \"bob\");";
        std::cout << "Query: ";
        std::getline(std::cin, query);
        auto lexer = std::make_shared<Lexer>();
        lexer->lex(query);

        Parser parser(storage, lexer);
        Statement stmt;
        parser.parse(&stmt);

        QueryVM vm(storage);
        vm.eval_stmt(&stmt);
        row_t row;
        auto beg = std::chrono::system_clock::now();
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
        auto end = std::chrono::system_clock::now();
        std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count() << "ms" << std::endl;
        vm.reset();
        //  return 0;
    }

    return 0;
}

#pragma clang diagnostic pop