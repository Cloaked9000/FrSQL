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
		: query(query), expected(std::move(expected))
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
	sql.exec(query.query, [&](const row_t& args){val.callback(args);});
	ASSERT_EQ(val.rows, query.expected);
}

INSTANTIATE_TEST_SUITE_P(
	SelectIntermediate,
	QueryTextFixture,
	::testing::Values(
		Expected("SELECT 10;", { {Variable(10)} }),
		Expected("SELECT -10;", { {Variable(-10)} }),
		Expected("SELECT \"bob\";", { {Variable("bob")} }),
		Expected("SELECT 10, 20;", { {Variable(10), Variable(20)} }),
		Expected("SELECT 10, \"hey\", 25;", { {Variable(10), Variable("hey"), Variable(25)} }),
		Expected("SELECT 10 * 20 * 3, 25 / 2, 5 + 5 * 2;", { {Variable(600), Variable(12), Variable(15)} }),
		Expected("SELECT 10 = 10;", { {Variable(1)} }),
		Expected("SELECT 10 = 11;", { {Variable(0)} }),
		Expected("SELECT 10 > 10;", { {Variable(0)} }),
		Expected("SELECT 11 > 10;", { {Variable(1)} }),
		Expected("SELECT 10 < 10;", { {Variable(0)} }),
		Expected("SELECT 9 < 10;", { {Variable(1)} }),
		Expected("SELECT 10 + 1 = 11;", { {Variable(1)} }),
		Expected("SELECT 10 + 2 = 11;", { {Variable(0)} }),
		Expected("SELECT NOT 10;", { {Variable(0)} }),
		Expected("SELECT NOT 0;", { {Variable(1)} }),
		Expected("SELECT NOT 10 = 10;", { {Variable(0)} }),
		Expected("SELECT NOT 10 = 11;", { {Variable(1)} }),
		Expected("SELECT 10 IN (10, 20, 15);", { {Variable(1)} }),
		Expected("SELECT 10 IN (20, 10, 15);", { {Variable(1)} }),
		Expected("SELECT 10 IN (20, 15, 10);", { {Variable(1)} }),
		Expected("SELECT 10 IN (10);", { {Variable(1)} }),
		Expected("SELECT 10 IN (11);", { {Variable(0)} }),
		Expected("SELECT 10 IN (11, 15);", { {Variable(0)} }),
		Expected("SELECT \"hello\" IN (\"bob\", \"garry\");", { {Variable(0)} }),
		Expected("SELECT \"hello\" IN (\"bob\", \"hello\");", { {Variable(1)} }),
		Expected("SELECT 1 IN (SELECT 5 in(10, 5));", { {Variable(1)} }),
		Expected("SELECT 1 IN (SELECT 5 in(7, 5));", { {Variable(1)} }),
		Expected("SELECT 7 IN (SELECT 5, 7, 8);", { {Variable(1)} }),
		Expected("SELECT 10 IN (SELECT 5, 7, 8);", { {Variable(0)} }),
		Expected("SELECT 50 NOT IN (70, 50, 100)", { {Variable(0)} }),
		Expected("SELECT 50 NOT IN (70, 60, 100);", { {Variable(1)} })
	));
