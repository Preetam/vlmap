#!/bin/sh

okay=$(valgrind --tool=memcheck --leak-check=full --undef-value-errors=no ./test 2>&1 |  grep -c "in use at exit: 0 bytes in 0 blocks")

if [ "$okay" = "1" ]; then
	return 0
fi
echo "Memory leaks found!"
return 1