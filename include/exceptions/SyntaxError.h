#ifndef COMPILER_SYNTAXERROR_H
#define COMPILER_SYNTAXERROR_H


#include "DatabaseError.h"

class SyntaxError : public DatabaseError
{
public:
    SyntaxError(const std::string &msg)
    : DatabaseError(msg)
    {}
};

#endif //COMPILER_SYNTAXERROR_H
