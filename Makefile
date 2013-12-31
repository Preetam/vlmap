clean:
	rm ./test/valgrind_test

test:
	gcc -o ./test/valgrind_test -g vlmap.c ./test/test.c
	cd test && ./run_valgrind.sh

install:
	cp vlmap.h /usr/include/
	gcc -shared -fPIC vlmap.c -o /usr/lib/libvlmap.so

.PHONY: clean test install
