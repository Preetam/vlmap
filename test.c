#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vlmap.h"

int main() {
	uint8_t* key = "foo";
	int keylength = 3;
	uint8_t* val = "bar";
	int vallength = 3;

	vlmap* m = vlmap_create();

	printf("Inserting `foo'\n");

	vlmap_insert(m, vlmap_version(m), key, keylength, val, vallength);

	uint8_t* key1 = "a";
	uint8_t* key2 = "d";
	uint8_t* key3 = "b";
	uint8_t* key4 = "c";
	uint8_t* key5 = "0";

	vlmap_insert(m, vlmap_version(m), key1, 1, val, vallength);
	vlmap_insert(m, vlmap_version(m), key2, 1, val, vallength);
	vlmap_insert(m, vlmap_version(m), key3, 1, val, vallength);
	vlmap_insert(m, vlmap_version(m), key4, 1, val, vallength);
	vlmap_insert(m, vlmap_version(m), key5, 1, val, vallength);

	printf("Incrementing version\n");

	// version is now at 1.
	vlmap_version_increment(m);

	// Remove "foo" at version 1.
	vlmap_remove(m, vlmap_version(m), key, strlen(key));

	// version is now at 2.
	vlmap_version_increment(m);

	uint8_t* value;
	int valuelength;

	//vlmap_print(m);

	//vlmap_print(m, 0);
	//vlmap_print(m, 1);
	//vlmap_print(m, 2);

	// Get "foo" at version 2 -- this should be an error.
	int err = vlmap_get(m, vlmap_version(m), key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s at version %d.\n", valuelength, value, (int)vlmap_version(m));
		free(value);
	} else {
		printf("`foo' is not present at version %d.\n", (int)vlmap_version(m));
	}

	err = vlmap_get(m, 0, key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s at version %d.\n", valuelength, value, 0);
		free(value);
	} else {
		printf("`foo' is not present at version %d.\n", 0);
	}

	err = vlmap_get(m, 1, key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s at version %d.\n", valuelength, value, 1);
		free(value);
	} else {
		printf("`foo' is not present at version %d.\n", 1);
	}

	vlmap_iterator* i = vlmap_create_iterator(m, 1, "\x00", 1, "\xff", 1);

	do {
		if(i != NULL)
			print_node(i->root);
		else
			break;
	} while(vlmap_iterator_next(i));

	if(i != NULL)
		vlmap_iterator_destroy(i);

	vlmap_destroy(m);

	exit(0);
}
