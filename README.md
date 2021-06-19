# FrSQL
An experimental - and very limited - SQL implementation for fun and experimentation, comprised of a lexer, parser, and virtual machine.

## Supported Features:
* UPDATE, DELETE, SELECT, INSERT
* CREATE/DELETE TABLE
* SUBQUERIES
* LIMIT
* (NOT) IN
* SHOW
* DESC

## Supported Example Queries:
```sql
CREATE TABLE admin(id INT, user_id INT);
INSERT INTO admin (id, user_id) VALUES(1, 1), (1, 2);


CREATE TABLE user(id INT, name STRING, age INT);
INSERT INTO user VALUES(1, "James", 16);
INSERT INTO user (id, name, age) VALUES (4, "Adam", 17), (3, "Terry", 20), (2, "Jessica", 21);

SELECT * FROM user WHERE id IN (SELECT user_id FROM admin);
SELECT name, age * 2 FROM user WHERE age % 2 = 0;

UPDATE user SET name = "Big Terry", age = age + 1 WHERE id = 3;
DELETE FROM user WHERE id = 4;
SELECT * FROM user;
```

## Screenshots:
![alt text](https://raw.githubusercontent.com/Cloaked9000/FrSQL/main/screenshots/example1.png)