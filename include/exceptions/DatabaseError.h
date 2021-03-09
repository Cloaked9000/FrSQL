#ifndef FRSQL_DATABASEERROR_H
#define FRSQL_DATABASEERROR_H


#include <stdexcept>

class DatabaseError : public std::runtime_error
{
public:
    explicit DatabaseError(const std::string& msg)
        : std::runtime_error(msg)
    {}
};

#endif //FRSQL_DATABASEERROR_H
