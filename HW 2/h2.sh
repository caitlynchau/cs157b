echo 'starting h2.sql'
db2 -td"^" -f h2.sql > res.out
echo 'finished h2.sql'

echo 'starting test.sql'
db2 -td"^" -f test.sql >> res.out
echo 'finished test.sql'
