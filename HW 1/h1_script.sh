echo 'executing db2 connect to sample'
db2 connect to sample
echo 'finished db2 connect to sample'
echo

echo 'executing db2 prep h1.sqc'
db2 prep h1.sqc
echo 'finished db2 prep h1.sqc'
echo

echo 'executing gcc -I../sqllib/include -c h1.c'
gcc -I../sqllib/include -c h1.c
echo 'finished gcc -I../sqllib/include -c h1.c'
echo

echo 'executing gcc -o h1 h1.o -L../sqllib/lib  -ldb2'
gcc -o h1 h1.o -L../sqllib/lib  -ldb2
echo 'finished gcc -o h1 h1.o -L../sqllib/lib  -ldb2'
echo

echo 'executing ./h1 sample'
./h1 sample
echo 'finished ./h1 sample'
echo