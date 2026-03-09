#!/bin/bash
set -euo pipefail

TOOL="bazel-bin/service/tools/kv/api_tools/kv_service_tools"
CONFIG="service/tools/config/interface/service.config"
LOGFILE="sql_test.log"

# Colors
BLUE="\033[1;34m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
RESET="\033[0m"

run_sql() {
  echo -e "${GREEN}>>> $1${RESET}"
  echo "---- $1 ----" >> $LOGFILE
  $TOOL --config $CONFIG --cmd sql --sql "$2" | tee -a $LOGFILE
  echo "" >> $LOGFILE
}

echo -e "${BLUE}===== Starting SQL Test Suite =====${RESET}"
echo "SQL Test Log - $(date)" > $LOGFILE
echo "" >> $LOGFILE

# 1. CREATE TABLES
run_sql "Create table: users" \
"CREATE TABLE users (id INTEGER, name VARCHAR, age INTEGER);"

run_sql "Create table: orders" \
"CREATE TABLE orders (order_id INTEGER, user_id INTEGER, amount DOUBLE, status VARCHAR);"

# 2. INSERT DATA
run_sql "Insert into users" \
"INSERT INTO users VALUES 
 (1,'Alice',30),
 (2,'Bob',22),
 (3,'Charlie',27),
 (4,'Diana',35);"

run_sql "Insert into orders" \
"INSERT INTO orders VALUES
 (101,1,250.50,'shipped'),
 (102,1,120.00,'processing'),
 (103,2,75.00,'cancelled'),
 (104,3,500.00,'shipped'),
 (105,3,20.00,'processing');"

# 3. BASIC SELECTS
run_sql "Select all users" \
"SELECT * FROM users;"

run_sql "Filter: age > 25" \
"SELECT name, age FROM users WHERE age > 25;"

# 4. ORDER BY
run_sql "Order users by age DESC" \
"SELECT * FROM users ORDER BY age DESC;"

# 5. AGGREGATIONS
run_sql "Count users" \
"SELECT COUNT(*) AS total_users FROM users;"

run_sql "Average order amount" \
"SELECT AVG(amount) AS avg_order FROM orders;"

run_sql "Total spent per user" \
"SELECT user_id, SUM(amount) AS total_spent FROM orders GROUP BY user_id;"

# 6. JOIN TESTS
run_sql "JOIN: users + orders" \
"SELECT u.name, o.order_id, o.amount 
 FROM users u 
 JOIN orders o ON u.id = o.user_id;"

run_sql "JOIN + filter amount > 100" \
"SELECT u.name, o.amount 
 FROM users u 
 JOIN orders o ON u.id = o.user_id 
 WHERE o.amount > 100;"

run_sql "LEFT JOIN" \
"SELECT u.id, u.name, o.order_id 
 FROM users u 
 LEFT JOIN orders o ON u.id = o.user_id 
 ORDER BY u.id;"

# 7. UPDATE / DELETE
run_sql "Update Bob's age" \
"UPDATE users SET age = age + 1 WHERE id = 2;"

run_sql "Delete cancelled orders" \
"DELETE FROM orders WHERE status = 'cancelled';"

# 8. GROUPED JOIN AGGREGATE
run_sql "Total spent by each user (ordered)" \
"SELECT u.name, SUM(o.amount) AS total_spent 
 FROM users u 
 JOIN orders o ON u.id = o.user_id 
 GROUP BY u.id, u.name 
 ORDER BY total_spent DESC;"

# 9. DISTINCT / LIKE / BETWEEN / LIMIT
run_sql "Distinct order statuses" \
"SELECT DISTINCT status FROM orders;"

run_sql "Names starting with A" \
"SELECT * FROM users WHERE name LIKE 'A%';"

run_sql "Age between 25 and 35" \
"SELECT * FROM users WHERE age BETWEEN 25 AND 35;"

run_sql "Top orders (limit + offset)" \
"SELECT * FROM orders ORDER BY amount DESC LIMIT 2 OFFSET 1;"

# 10. STRING FUNCTIONS
run_sql "String functions test" \
"SELECT UPPER(name), LENGTH(name) FROM users;"

# 11. TRANSACTION BLOCK
run_sql "Transaction test" \
"BEGIN TRANSACTION;
 INSERT INTO users VALUES (10, 'Eva', 40);
 UPDATE users SET age = 41 WHERE id = 10;
 COMMIT;"

# 12. ERROR TESTS (expected failure)
echo -e "${YELLOW}>>> Testing expected errors (these should fail)${RESET}"

run_sql "Malformed row insert (should error)" \
"INSERT INTO users VALUES ('bad','data',123);" || true

run_sql "Select from invalid table (should error)" \
"SELECT * FROM nonexistent;" || true

echo -e "${BLUE}===== Now cleaning up =====${RESET}"

# 13. CLEANUP
run_sql "Drop table: orders" \
"DROP TABLE orders;"

run_sql "Drop table: users" \
"DROP TABLE users;"

echo -e "${BLUE}===== SQL Test Suite Complete =====${RESET}"
echo "Full output written to $LOGFILE"
