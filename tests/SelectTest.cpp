#include <gtest/gtest.h>
#include <frsql.h>

inline void PrintTo(const std::vector<row_t> &rows, ::std::ostream* os)
{
	*os << "{";
	for (size_t a = 0; a < rows.size(); a++)
	{
		*os << "{";
		for (size_t b = 0; b < rows[a].size(); b++)
		{
			if (rows[a][b].type == Variable::Type::INT)
			{
				*os << rows[a][b].store.int64;
			}
			else if (rows[a][b].type == Variable::Type::STRING)
			{
				*os << std::string_view(rows[a][b].store.str, rows[a][b].store.len);
			}
			if (b != rows[a].size() - 1)
			{
				*os << ", ";
			}
		}
		if (a != rows.size() - 1)
		{
			*os << ", ";
		}
		*os << "}";
	}
	*os << "}";
}

struct ValStore
{
	void callback(const row_t &row)
	{
		rows.emplace_back( row );
	}

	std::vector<row_t> rows;
};

struct Expected
{
	Expected(std::string_view query, std::vector<row_t> expected)
		: query(std::move(query)), expected(std::move(expected))
	{

	}
		
	std::string_view query;
	std::vector<row_t> expected;
};

class QueryTextFixture :public ::testing::TestWithParam<Expected> 
{
protected:
	Frsql sql;
};

TEST_P(QueryTextFixture, test_select) {
	Expected query = GetParam();
	ValStore val;
	sql.exec(query.query, std::bind(&ValStore::callback, &val, std::placeholders::_1));
	ASSERT_EQ(val.rows, query.expected);
}

INSTANTIATE_TEST_SUITE_P(
	SelectIntermediate,
	QueryTextFixture,
	::testing::Values(
		Expected("SELECT 10;", { {Variable(10)} }),
		Expected("SELECT -10;", { {Variable(-10)} }),
		Expected("SELECT \"bob\";", { {Variable("bob", 3)} }),
		Expected("SELECT 10, 20;", { {Variable(10), Variable(20)} }),
		Expected("SELECT 10, \"hey\", 25;", { {Variable(10), Variable("hey", 3), Variable(25)} }),
		Expected("SELECT 10 * 20 * 3, 25 / 2, 5 + 5 * 2;", { {Variable(600), Variable(12), Variable(15)} })
	));
