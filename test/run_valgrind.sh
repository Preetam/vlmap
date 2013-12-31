#!/bin/sh

valgrind_out=$(valgrind --tool=memcheck --leak-check=full --undef-value-errors=no ./valgrind_test 2>&1)
okay=$(echo $valgrind_out |  grep -c "in use at exit: 0 bytes in 0 blocks")

if [ "$okay" = "1" ]; then
	return 0
fi
echo "$valgrind_out"
return 1
