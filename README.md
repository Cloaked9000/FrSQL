# FrSQL
An experimental - and very limited- SQL implementation for fun and experimentation, comprised of a lexer, parser, and virtual machine.

## (Somewhat) Supported Statement Types:
* SELECT
* INSERT
* SHOW
* DESC
* CREATE
* DELETE

## Supported Example Queries:
```sql
CREATE TABLE user(id INT, name STRING, age INT);
INSERT INTO user VALUES(1, "Bob", 15);
INSERT INTO user VALUES(2, "Andy", 16);
SELECT (5 * 5), name FROM user WHERE age > (SELECT age FROM tab WHERE name == "Bob" LIMIT 1);
DELETE FROM user WHERE age % 2 == 0;
SELECT * FROM user;
```