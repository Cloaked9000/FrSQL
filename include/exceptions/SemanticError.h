#ifndef COMPILER_SEMANTICERROR_H
#define COMPILER_SEMANTICERROR_H

#include <stdexcept>

class SemanticError : public std::runtime_error
{
public:
    explicit SemanticError(const std::string &msg)
    : std::runtime_error(msg)
    {}
};


#endif //COMPILER_SEMANTICERROR_H
