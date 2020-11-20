//
// Created by fred.nicolson on 28/02/2020.
//

#ifndef TESTDB_OPCODE_H
#define TESTDB_OPCODE_H

enum class Opcode : uint8_t
{
    PUSH_INT64 = 0,
    PUSH_STRING = 1,
    MULT = 2,
    DIV = 3,
    MOD = 4,
    SUB = 5,
    ADD = 6,
    COMP_NE = 7,
    COMP_EQ = 8,
    COMP_GT = 9,
    COMP_LT = 10,
    LOAD_COL = 11,
    QUIT = 12
};

#endif //TESTDB_OPCODE_H
