#ifndef COMPILER_SEMANTICERROR_H
#define COMPILER_SEMANTICERROR_H

#include "DatabaseError.h"

class SemanticError : public DatabaseError
{
public:
    SemanticError(const std::string &msg)
    : DatabaseError(msg)
    {}
};


#endif //COMPILER_SEMANTICERROR_H
