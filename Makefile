clean:
	rm test

test:
	gcc -o test -g vlmap.c test.c
	./run_valgrind.sh

.PHONY: clean test