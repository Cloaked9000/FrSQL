cmake_minimum_required(VERSION 3.15)
project(TestDB)
include_directories(include)
set(CMAKE_CXX_STANDARD 20)

# Build options
option(BUILD_TESTS "Build FrSQL tests" OFF)

set(SOURCE_FILES
        main.cpp
        src/Parser.cpp
        src/Lexer.cpp
        include/table/TableMetadata.h
        include/Variable.h
        include/Parser.h
        include/Lexer.h
        include/exceptions/SyntaxError.h
        include/exceptions/SemanticError.h include/Statement.h src/QueryVM.cpp include/QueryVM.h include/Opcode.h src/table/Table.cpp include/table/Table.h include/Database.h src/Database.cpp "include/frsql.h" "include/exceptions/DatabaseError.h" src/Stack.cpp include/Stack.h src/btree/BTree.cpp include/btree/BTree.h include/filesystem/Filesystem.h src/filesystem/BasicFilesystem.cpp include/filesystem/BasicFilesystem.h include/filesystem/FilesystemBacking.h include/btree/Node.h src/btree/Node.cpp src/btree/NodeStore.cpp include/btree/NodeStore.h src/table/RowStorage.cpp include/table/RowStorage.h src/table/TableStorage.cpp include/table/TableStorage.h include/serializers/Serializer.h include/serializers/Stl.h include/serializers/Table.h include/serializers/FilehandleSerializerAdapter.h)

 
if(BUILD_TESTS)
    add_definitions(-DBUILD_TESTS)
    set(SOURCE_FILES ${SOURCE_FILES} tests/SelectTest.cpp tests/StackTest.cpp tests/TestUtils.h tests/BTreeTest.cpp tests/FilesystemTest.cpp)

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
add_link_options(TestDB -fsanitize=address)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    target_compile_options(TestDB PRIVATE /EHa)
endif()

if(BUILD_TESTS)
    target_link_libraries(TestDB gtest_main)
endif()