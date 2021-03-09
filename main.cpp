#include <iostream>
#include <Parser.h>
#include <QueryVM.h>
#include <memory>
#include <string>
#include <chrono>
#include <frsql.h>

#ifdef BUILD_TESTS
#include <gtest/gtest.h>
#endif
#include <frsql.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main(int argc, char **argv)
{
#ifdef BUILD_TESTS
    if(argc > 1 && strcmp(argv[1], "test") == 0)
    {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
#endif

    Frsql frsql;
    while(true)
    {
        std::string query;
        std::cout << "Query: ";
        std::getline(std::cin, query);

#ifdef BUILD_TESTS
        if (query == "test")
        {
            ::testing::InitGoogleTest(&argc, argv);
            return RUN_ALL_TESTS();
        }
#endif

        try
        {
            frsql.exec(query, [&](const row_t& row) {
                for (auto& r : row)
                {
                    if (r.type == Variable::Type::INT)
                    {
                        std::cout << r.store.int64 << ", ";
                    }
                    else if (r.type == Variable::Type::STRING)
                    {
                        std::cout << std::string_view(r.store.str, r.store.len) << ", ";
                    }

                }

                if (!row.empty())
                {
                    std::cout << "\n" << std::endl;
                }
                });
        }
        catch (const DatabaseError &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}

#pragma clang diagnostic pop