clean:
	rm test

test:
	gcc -o valgrind_test -g vlmap.c test.c
	./run_valgrind.sh

install:
	cp vlmap.h /usr/include/
	gcc -shared -fPIC vlmap.c -o /usr/lib/libvlmap.so

.PHONY: clean test