#ifndef COMPILER_VARIABLE_H
#define COMPILER_VARIABLE_H
#include <string>
#include <cstring>
#include <exceptions/SemanticError.h>

class Variable
{
public:

    enum class Type
    {
        INT = 0,
        STRING = 1
    };

    Variable()
    : type(Type::INT)
    {

    }

    Variable(const Variable &o)
    : type(o.type),
      store(o.store)
    {

    }

    explicit Variable(int64_t val)
    {
        store.int64 = val;
        type = Type::INT;
    }

    explicit Variable(const char *str, size_t len)
    {
        store.str = str;
        store.len = len;
        type = Type::STRING;
    }

    Type type;
    union store
    {
        int64_t int64;
        struct
        {
            const char *str;
            uint64_t len;
        };
    } store{};

};

inline bool operator==(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        return false;
    if(a.type == Variable::Type::INT)
        return a.store.int64 == b.store.int64;
    if(a.type == Variable::Type::STRING)
        return strcmp(a.store.str, b.store.str) == 0;
    abort();
}

inline bool operator!=(const Variable &a, const Variable &b)
{
    return !(a == b);
}

inline Variable operator+(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't + incompatible types");
    if(a.type == Variable::Type::INT)
        return Variable(a.store.int64 + b.store.int64);
    if(a.type == Variable::Type::STRING)
        throw SemanticError("Can't join strings yet! :(");
    abort();
}

inline Variable operator-(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't + incompatible types");
    if(a.type == Variable::Type::INT)
        return Variable(a.store.int64 - b.store.int64);
    if(a.type == Variable::Type::STRING)
        throw SemanticError("Can't subtract strings!");
    abort();
}

inline Variable operator*(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't multiply incompatible types");
    if(a.type != Variable::Type::INT)
        throw SemanticError("Can't multiply non-integers");
    return Variable(a.store.int64 * b.store.int64);
}

inline Variable operator/(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't divide incompatible types");
    if(a.type != Variable::Type::INT)
        throw SemanticError("Can't divide non-integers");
    return Variable(a.store.int64 / b.store.int64);
}

inline Variable operator%(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't mod incompatible types");
    if(a.type != Variable::Type::INT)
        throw SemanticError("Can't mod non-integers");
    return Variable(a.store.int64 % b.store.int64);
}

inline Variable operator>(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't > incompatible types");
    if(a.type != Variable::Type::INT)
        throw SemanticError("Can't > non-integers");
    return Variable(a.store.int64 > b.store.int64);
}

inline Variable operator<(const Variable &a, const Variable &b)
{
    if(a.type != b.type)
        throw SemanticError("Can't < incompatible types");
    if(a.type != Variable::Type::INT)
        throw SemanticError("Can't < non-integers");
    return Variable(a.store.int64 < b.store.int64);
}

#endif //COMPILER_VARIABLE_H
