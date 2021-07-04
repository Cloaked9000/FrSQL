#ifndef TESTDB_DATABASE
#define TESTDB_DATABASE

#include <memory>
#include <string_view>
#include "Table.h"

class Database
{
public:
	Database();
	std::shared_ptr<Table> load_table(tid_t id);
	std::vector<tid_t> list_table_ids();
	std::shared_ptr<Table> create_table(std::string name, std::vector<ColumnMetadata> columns);
	[[nodiscard]] tid_t lookup_table(std::string_view name) const;
private:
	tid_t current_tid;
	std::unordered_map<tid_t, std::shared_ptr<Table>> tables;
	std::unordered_map<std::string_view, tid_t> table_lookup;
};

#endif // TESTDB_DATABASE