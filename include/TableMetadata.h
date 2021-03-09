#ifndef TESTDB_TABLEMETADATA_H
#define TESTDB_TABLEMETADATA_H

#include <vector>
#include <string>

#define ID_NONE 0
typedef size_t tid_t;
typedef size_t cid_t;

enum class Constraint
{
	NOT_NULL = 0,
};

struct ColumnMetadata
{
	cid_t id;
	std::string name;
	std::string type;
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

	tid_t id;
	std::string name;
	std::vector<ColumnMetadata> columns;
};

#endif // TESTDB_TABLEMETADATA_H