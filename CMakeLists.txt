cmake_minimum_required(VERSION 3.15)
project(TestDB)
include_directories(include)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        main.cpp
        src/Parser.cpp
        src/Lexer.cpp
        include/Variable.h
        include/Parser.h
        include/Lexer.h
        include/exceptions/SyntaxError.h
        include/exceptions/SemanticError.h include/Statement.h src/QueryVM.cpp include/QueryVM.h include/Opcode.h src/Table.cpp include/Table.h src/StorageEngine.cpp include/StorageEngine.h)

add_executable(TestDB ${SOURCE_FILES})