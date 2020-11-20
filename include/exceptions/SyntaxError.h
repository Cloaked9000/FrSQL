#ifndef COMPILER_SYNTAXERROR_H
#define COMPILER_SYNTAXERROR_H


#include <stdexcept>

class SyntaxError : public std::runtime_error
{
public:
    explicit SyntaxError(const std::string &msg)
    : std::runtime_error(msg)
    {}
};

#endif //COMPILER_SYNTAXERROR_H
