cmake_minimum_required(VERSION 3.15)
project(TestDB)
include_directories(include)
set(CMAKE_CXX_STANDARD 20)

# Build options
option(BUILD_TESTS "Build FrSQL tests" ON)

set(SOURCE_FILES
        main.cpp
        src/Parser.cpp
        src/Lexer.cpp
        include/TableMetadata.h
        include/Variable.h
        include/Parser.h
        include/Lexer.h
        include/exceptions/SyntaxError.h
        include/exceptions/SemanticError.h include/Statement.h src/QueryVM.cpp include/QueryVM.h include/Opcode.h src/Table.cpp include/Table.h include/Database.h src/Database.cpp "include/frsql.h" "include/exceptions/DatabaseError.h" src/Stack.cpp include/Stack.h)

 
if(BUILD_TESTS)
    add_definitions(-DBUILD_TESTS)
    set(SOURCE_FILES ${SOURCE_FILES} tests/SelectTest.cpp tests/StackTest.cpp tests/TestUtils.h)

    configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . RESULT_VARIABLE result WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build . RESULT_VARIABLE result WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src ${CMAKE_CURRENT_BINARY_DIR}/googletest-build EXCLUDE_FROM_ALL)
endif()

add_executable(TestDB ${SOURCE_FILES})
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    target_compile_options(TestDB PRIVATE /EHa)
endif()

if(BUILD_TESTS)
    target_link_libraries(TestDB gtest_main)
endif()