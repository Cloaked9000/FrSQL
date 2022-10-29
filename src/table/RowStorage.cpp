//
// Created by fred on 29/06/2022.
//

#include <cassert>
#include "table/RowStorage.h"
#include "Variable.h"

RowStorage::RowStorage(TableMetadata metadata, Filesystem::Handle index, Filesystem::Handle data)
: metadata(std::move(metadata)), index(std::move(index)), data(std::move(data))
{
    row_size = this->metadata.get_column_count() * sizeof(rid_t);
}

rid_t RowStorage::store(const std::vector<Variable> &row)
{
    for(const auto &val : row)
    {
        switch(val.type)
        {
            case Variable::Type::INT:
                data.write(reinterpret_cast<const char *>(&val.store.int64), sizeof(val.store.int64));
                break;
            case Variable::Type::STRING:
                abort(); // todo implement
                break;
        }
    }
    assert(data.tell() % row_size == 0);
    return (data.tell() / row_size) - 1;
}

void RowStorage::erase(rid_t row_id)
{
    index.write(reinterpret_cast<const char *>(&row_id), sizeof(row_id));
}

std::vector<Variable> RowStorage::load(rid_t row_id)
{
    if(data.seek(row_id * row_size) != row_id * row_size)
    {
        throw std::runtime_error("Failed to seek to row");
    }

    size_t bytes_read = 0;
    std::vector<Variable> row;
    for(size_t a = 0; a < metadata.get_column_count(); a++)
    {
        row.emplace_back();
        row.back().type = metadata.columns[a].type;
        switch(metadata.columns[a].type)
        {
            case Variable::Type::INT:
                bytes_read += data.read(reinterpret_cast<char *>(&row.back().store.int64), sizeof(row.back().store.int64));
                break;
            case Variable::Type::STRING:
                abort(); // todo implement
                break;
        }
    }

    if(bytes_read != row_size)
    {
        throw std::runtime_error("Failed to read row");
    }

    return row;
}

void RowStorage::update(rid_t row_id, const std::vector<Variable> &row)
{

}
