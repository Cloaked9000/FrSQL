#include <iostream>
#include <string>
#include "btree/BTree.h"
#include "Lexer.h"
#include "filesystem/Filesystem.h"
#include "filesystem/BasicFilesystem.h"
#include "frsql.h"

#ifdef BUILD_TESTS
#include <gtest/gtest.h>
#include <chrono>
#endif

#include <ranges>

int main(int argc, char **argv)
{
#ifdef BUILD_TESTS
        if(argc > 1 && strstr(argv[1], "test") != nullptr)
        {
            ::testing::InitGoogleTest(&argc, argv);
            return RUN_ALL_TESTS();
        }
#endif

    std::unique_ptr<FilesystemBacking> backing;
    if(argc > 1)
    {
        const std::string filepath(argv[1]);
        backing = std::make_unique<DiskBacking>();
        if(!backing->open(filepath, false))
        {
            if(!backing->open(filepath, true))
            {
                std::cout << "Failed to open/create database file '" << filepath << "'\n";
                return EXIT_FAILURE;
            }
            else
            {
                std::cout << "Created new database file '" << filepath << "'\n";
            }
            BasicFilesystem::Format(backing);
        }
        else
        {
            std::cout << "Opened existing database file '" << filepath << "'\n";
        }
    }
    else
    {
        std::cout << "No database filepath provided. Using an in-memory backing.\n";
        backing = std::make_unique<MemoryBacking>();
        BasicFilesystem::Format(backing);
    }

    Frsql frsql(std::make_unique<BasicFilesystem>(std::move(backing)));


//    auto begin = std::chrono::steady_clock::now();
//    for(size_t a = 0; a < 9000000; a++)
//    {
//        frsql.exec("SELECT 10 IN (5, 7, 8, 10, 11, 15, 17)");
//    }
//    auto end = std::chrono::steady_clock::now();
//    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
//    return 0;

    while(true)
    {
        std::string query;
        std::cout << "Query: ";
        std::getline(std::cin, query);
        if(query.empty())
        {
            continue;
        }

#ifdef BUILD_TESTS
        if (query == "test")
        {
            ::testing::InitGoogleTest(&argc, argv);
            return RUN_ALL_TESTS();
        }
#endif

        if(query == "exit")
        {
            break;
        }

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
                    std::cout << "\n";
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