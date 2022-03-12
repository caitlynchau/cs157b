echo '[h2.sql] creating triggers'
db2 -td"^" -f h2.sql > res.out
echo '[h2.sql] finished creating triggers'

echo '[test.sql] running test cases'
db2 -td"^" -f test.sql >> res.out
echo '[test.sql] finished running test cases'
