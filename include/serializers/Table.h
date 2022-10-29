//
// Created by fred on 29/10/2022.
//

#ifndef TESTDB_TABLE_H
#define TESTDB_TABLE_H
#include "Serializer.h"

inline void serialize(serializer::Serializer &serializer, const ColumnMetadata &str)
{
    serializer << str.id << str.name << str.type << str.constraints;
}

inline bool deserialize(serializer::Serializer &serializer, ColumnMetadata &str)
{
    return serializer.extract(str.id) && serializer.extract(str.name) && serializer.extract(str.type) && serializer.extract(str.constraints);
}

inline void serialize(serializer::Serializer &serializer, const TableMetadata &meta)
{
    serializer << meta.id << meta.name << meta.columns;
}

inline bool deserialize(serializer::Serializer &serializer, TableMetadata &meta)
{
    return serializer.extract(meta.id) && serializer.extract(meta.name) && serializer.extract(meta.columns);
}

#endif //TESTDB_TABLE_H
