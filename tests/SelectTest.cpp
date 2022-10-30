#include "TestUtils.h"
#include "filesystem/FilesystemBacking.h"
#include "filesystem/BasicFilesystem.h"

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

inline void PrintTo(const Expected &expected, ::std::ostream* os)
{
    *os << expected.query;
}

class SelectTest :public ::testing::TestWithParam<Expected>
{
public:
    SelectTest()
    {
        std::unique_ptr<FilesystemBacking> backing = std::make_unique<MemoryBacking>();
        BasicFilesystem::Format(backing);
        auto fs = std::make_unique<BasicFilesystem>(std::move(backing));
        sql = std::make_unique<Frsql>(std::move(fs));

        sql->exec("CREATE TABLE user (id INT, name STRING, age INT);");
        sql->exec("CREATE TABLE admin (id INT, user_id INT);");
        sql->exec(R"(INSERT INTO user (id, name, age) VALUES (1, "Garry", 10), (2, "Barry", 15), (3, "Larry", 5);)");
        sql->exec(R"(INSERT INTO admin (id, user_id) VALUES(1, 2), (1, 3);)");
    }
protected:
	std::unique_ptr<Frsql> sql;
};

TEST_P(SelectTest, test_select) {
	Expected query = GetParam();
	ValStore val;
	sql->exec(query.query, [&](const row_t& args){val.callback(args);});
	ASSERT_EQ(val.rows, query.expected);
}

INSTANTIATE_TEST_SUITE_P(
	SelectIntermediate,
	SelectTest,
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
		Expected("SELECT * FROM (SELECT 10, 20);", { {Variable(10), Variable(20)} })
    ));

INSTANTIATE_TEST_SUITE_P(
        SelectIN,
        SelectTest,
        ::testing::Values(
		Expected("SELECT 10 IN (10, 20, 15);", { {Variable(1)} }),
		Expected("SELECT 10 IN (20, 10, 15);", { {Variable(1)} }),
		Expected("SELECT 10 IN (20, 15, 10);", { {Variable(1)} }),
		Expected("SELECT 10 IN (10);", { {Variable(1)} }),
		Expected("SELECT 10 IN (11);", { {Variable(0)} }),
		Expected("SELECT 1 IN (1 IN (5, 6), 2 IN (3, 4));", { {Variable(0)} }),
		Expected("SELECT 1 IN (1 IN (5, 6), 2 IN (2, 4));", { {Variable(1)} }),
		Expected("SELECT 10 IN (11, 15);", { {Variable(0)} }),
		Expected("SELECT 10 + 5 IN (11, 15);", { {Variable(1)} }),
		Expected("SELECT 10 + 5 IN (10, 16);", { {Variable(0)} }),
		Expected("SELECT \"hello\" IN (\"bob\", \"garry\");", { {Variable(0)} }),
		Expected("SELECT \"hello\" IN (\"bob\", \"hello\");", { {Variable(1)} }),
		Expected("SELECT 1 IN (SELECT 5 in(10, 5));", { {Variable(1)} }),
		Expected("SELECT 1 IN (SELECT 5 in(7, 5));", { {Variable(1)} }),
		Expected("SELECT 7 IN (SELECT 5, 7, 8);", { {Variable(1)} }),
		Expected("SELECT 10 IN (SELECT 5, 7, 8);", { {Variable(0)} }),
		Expected("SELECT 50 NOT IN (70, 50, 100)", { {Variable(0)} }),
		Expected("SELECT 50 NOT IN (70, 60, 100);", { {Variable(1)} })
	));

INSTANTIATE_TEST_SUITE_P(
        SelectFromTable,
        SelectTest,
        ::testing::Values(
                Expected("SELECT id FROM user;", { {Variable(3) }, {Variable(2)}, {Variable(1)} }),
                Expected("SELECT id, name FROM user;", { {Variable(3), Variable("Larry") }, {Variable(2), Variable("Barry")}, {Variable(1), Variable("Garry")} }),
                Expected("SELECT name FROM user WHERE id IN (SELECT user_id FROM admin);", { {Variable("Larry")}, {Variable("Barry")} }),
                Expected("SELECT * FROM user LIMIT 1;", { {Variable(3), Variable("Larry"), Variable(5)} }),
                Expected("SELECT id + 1 FROM user LIMIT 1;", { {Variable(4)} })
    ));

INSTANTIATE_TEST_SUITE_P(
        SelectLIMIT,
        SelectTest,
        ::testing::Values(
                Expected("SELECT id FROM user LIMIT 200;", { {Variable(3)}, {Variable(2)}, {Variable(1)} }),
                Expected("SELECT id FROM user LIMIT 1;", { {Variable(3)} }),
                Expected("SELECT id FROM user LIMIT 0;", {})
        ));

INSTANTIATE_TEST_SUITE_P(
        SelectOrdered,
        SelectTest,
        ::testing::Values(

        ));