#include "Database.h"
#include "filesystem/BasicFilesystem.h"

Database::Database(std::unique_ptr<Filesystem> filesystem_)
: filesystem(std::move(filesystem_))
{
    auto tableFile = filesystem->open("TABLE_STORAGE", true);
    tables = std::make_unique<TableStorage>(std::move(tableFile));
}

Database::~Database()
{

}

std::shared_ptr<Table> Database::load_table(tid_t id)
{
    auto meta = tables->load(id);
    return std::make_shared<TreeTable>(std::move(meta), filesystem.get());
}

std::vector<tid_t> Database::list_table_ids()
{
    return tables->list();
}

std::shared_ptr<Table> Database::create_table(std::string name, std::vector<ColumnMetadata> columns)
{
    TableMetadata meta(0, std::move(name), std::move(columns));
    meta.id = tables->create(meta);
    auto table = std::make_shared<TreeTable>(meta, filesystem.get());
    return table;
}

std::optional<tid_t> Database::lookup_table(const std::string_view name) const
{
    return tables->find(name);
}
