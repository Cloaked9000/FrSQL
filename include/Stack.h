//
// Created by fred on 19/03/2021.
//

#ifndef TESTDB_STACK_H
#define TESTDB_STACK_H


#include <vector>
#include <cassert>
#include "Variable.h"

template<typename Type>
class Stack
{
public:
    template<typename ...Args>
    inline Stack &push(Args &&...args)
    {
        stack.emplace_back(std::forward<Args>(args)...);
        return *this;
    }

    [[nodiscard]] inline Type pop()
    {
        assert(!stack.empty());
        Type var(std::move(stack.back()));
        stack.pop_back();
        return var;
    }

    inline void pop(size_t count)
    {
        assert(size() >= count);
        stack.erase(stack.end() - count, stack.end());
    }

    [[nodiscard]] inline size_t size() const
    {
        return stack.size();
    }

    [[nodiscard]] inline Type &top()
    {
        assert(!stack.empty());
        return stack.back();
    }

    inline void clear()
    {
        stack.clear();
    }

    [[nodiscard]] inline auto begin()
    {
        return stack.begin();
    }

    [[nodiscard]] inline auto end()
    {
        return stack.end();
    }

    [[nodiscard]] inline Type &operator[](size_t index)
    {
        assert(index < size());
        return stack[index];
    }

    [[nodiscard]] inline bool empty() const
    {
        return stack.empty();
    }

private:
    std::vector<Type> stack;
};


#endif //TESTDB_STACK_H
