//
// Created by fred on 29/10/2022.
//

#ifndef TESTDB_STL_H
#define TESTDB_STL_H
#include <vector>
#include <string>
#include "Serializer.h"
namespace serializer
{
    template<typename T>
    inline void serialize(serializer::Serializer &serializer, const std::vector<T> &vec)
    {
        const uint64_t count = vec.size();
        serializer << count;
        for(auto const &a : vec)
        {
            serializer << a;
        }
    }

    template<typename T>
    inline bool deserialize(serializer::Serializer &serializer, std::vector<T> &vec)
    {
        uint64_t count = 0;
        serializer >> count;
        vec.resize(count);
        for(uint64_t a = 0; a < count; a++)
        {
            serializer >> vec[a];
        }

        return true;
    }

    inline void serialize(serializer::Serializer &serializer, const std::string &vec)
    {
        const uint64_t count = vec.size();
        serializer << count;
        serializer.write(vec.data(), vec.size());
    }

    inline bool deserialize(serializer::Serializer &serializer, std::string &vec)
    {
        uint64_t count;
        serializer >> count;
        vec.resize(count);
        return serializer.read(vec.data(), vec.size());
    }
}





#endif //TESTDB_STL_H
