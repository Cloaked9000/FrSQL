//
// Created by fred.nicolson on 28/02/2020.
//

#ifndef TESTDB_OPCODE_H
#define TESTDB_OPCODE_H

enum class Opcode : uint8_t
{
    QUIT = 0,
    PUSH_INT64,
    PUSH_STRING,
    MULT,
    DIV,
    MOD,
    SUB,
    ADD,
    COMP_NE,
    COMP_EQ,
    COMP_GT,
    COMP_LT,
    LOAD_COL,
    LOAD_ALL,
    EXEC_SUBQUERY,
    FRAME_MARKER,
    FILTER_MUTUAL,
    FLIP,
};

#endif //TESTDB_OPCODE_H
