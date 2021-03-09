#include "Database.h"

Database::Database()
    : current_tid(0)
{

}

std::shared_ptr<Table> Database::load_table(tid_t id)
{
    return tables.at(id);
}

std::vector<tid_t> Database::list_table_ids()
{
    std::vector<tid_t> ret;
    for (const auto& tab : tables)
    {
        ret.emplace_back(tab.first);
    }

    return ret;
}

std::shared_ptr<Table> Database::create_table(std::string name, std::vector<ColumnMetadata> columns)
{   
    const tid_t tid = ++current_tid;
    TableMetadata meta(tid, std::move(name), std::move(columns));
    auto table = std::make_shared<Table>(std::move(meta));
    tables[tid] = table;
    table_lookup[table->get_metadata().name] = tid;
    return table;
}

tid_t Database::lookup_table(const std::string_view name) const
{
    return table_lookup.at(name);
}
