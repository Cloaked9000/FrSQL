#ifndef TESTDB_TABLEMETADATA_H
#define TESTDB_TABLEMETADATA_H

#include <vector>
#include <string>

typedef size_t tid_t;
typedef size_t cid_t;
typedef size_t rid_t;
#define ID_NONE std::numeric_limits<cid_t>::max()

enum class Constraint
{
	NOT_NULL = 0,
};

struct ColumnMetadata
{
	cid_t id;
	std::string name;
	Variable::Type type;
	std::vector<Constraint> constraints;
};

struct TableMetadata
{
	TableMetadata(tid_t id, std::string name, std::vector<ColumnMetadata> columns)
    : id(id),
      name(std::move(name)),
      columns(std::move(columns))
	{

	}

	cid_t get_column_index(std::string_view col_name) const
    {
	    for(const auto &col : columns)
        {
	        if(col.name == col_name)
            {
	            return col.id;
            }
        }

	    return ID_NONE;
    }

    size_t get_column_count() const
    {
	    return columns.size();
    }

    std::string_view get_column_name(rid_t row_id) const
    {
        return columns[row_id].name;
    }

	tid_t id;
	std::string name;
	std::vector<ColumnMetadata> columns;
};

#endif // TESTDB_TABLEMETADATA_H