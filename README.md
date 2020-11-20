# FrSQL
An experimental - and very limited- SQL implementation for fun and experimentation, comprised of a lexer, parser, and virtual machine.

## (Somewhat) Supported Statement Types:
* SELECT
* INSERT
* SHOW
* DESC

## (Somewhat) Supported Features:
* Logical/comparison operators
* Arithmetic
* Subqueries
* Limit

## Demo Queries:
```sql
SELECT (5 * 5), name FROM tab WHERE age > (SELECT age FROM tab WHERE name == "Bob" LIMIT 1);
```