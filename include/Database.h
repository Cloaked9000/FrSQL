#ifndef TESTDB_DATABASE
#define TESTDB_DATABASE

#include <memory>
#include <string_view>
#include "table/Table.h"
#include "table/TableStorage.h"

class Database
{
public:
	Database(std::unique_ptr<Filesystem> filesystem);
    ~Database();
	std::shared_ptr<Table> load_table(tid_t id);
	std::vector<tid_t> list_table_ids();
	std::shared_ptr<Table> create_table(std::string name, std::vector<ColumnMetadata> columns);
	[[nodiscard]] std::optional<tid_t> lookup_table(std::string_view name) const;

private:
    std::unique_ptr<Filesystem> filesystem;
    std::unique_ptr<TableStorage> tables;
};

#endif // TESTDB_DATABASE