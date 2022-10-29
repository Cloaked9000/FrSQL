//
// Created by fred on 17/07/2022.
//

#include "table/TableStorage.h"
#include "serializers/Stl.h"
#include "serializers/Table.h"
#include "serializers/FilehandleSerializerAdapter.h"
#include <string>
#include <algorithm>

class MyData
{
public:
    float bobby;
    int bob;
};


TableStorage::TableStorage(Filesystem::Handle index)
: data(std::move(index))
{
    serializer::Serializer serializer(std::make_unique<FilehandleSerializerAdapter>(data));

    if(!serializer.extract(metadata))
    {
        serializer.pack(metadata);
    }

    for(size_t a = 0; a < metadata.table_count; a++)
    {
        TableMetadata meta{};
        serializer >> meta;
        tables.emplace_back(std::move(meta));
    }
}

TableStorage::~TableStorage()
{
    data.seek(0);
    serializer::Serializer serializer(std::make_unique<FilehandleSerializerAdapter>(data));
    serializer << metadata;
    for(const auto &table : tables)
    {
        serializer << table;
    }
}

tid_t TableStorage::create(TableMetadata table)
{
    table.id = metadata.current_table_id++;
    tables.emplace_back(std::move(table));
    metadata.table_count++;
    return metadata.current_table_id;
}

void TableStorage::erase(tid_t table_id)
{
    auto iter = std::find_if(std::begin(tables), std::end(tables), [table_id](const auto &table) {
        return table.id == table_id;
    });
    if(iter != tables.end())
    {
        tables.erase(iter);
        metadata.table_count--;
    }
}

std::optional<tid_t> TableStorage::find(std::string_view name)
{
    auto iter = std::find_if(std::begin(tables), std::end(tables), [name](const auto &table) {
        return table.name == name;
    });
    return iter != tables.end() ? std::make_optional(iter->id) : std::nullopt;
}

std::vector<tid_t> TableStorage::list()
{
    std::vector<tid_t> ids;
    std::transform(std::begin(tables), std::end(tables), std::back_inserter(ids), [](const auto &elem) {
        return elem.id;
    });
    return ids;
}

TableMetadata TableStorage::load(tid_t table_id)
{
    auto iter = std::find_if(std::begin(tables), std::end(tables), [table_id](const auto &table) {
        return table.id == table_id;
    });
    return *iter;
}
