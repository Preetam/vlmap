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

	// set "foo" => "bar" at version 0.
	vlmap_insert(m, vlmap_version(m), key, keylength, val, vallength);

	// version is now at 1.
	vlmap_version_increment(m);

	// Remove "foo" at version 1.
	vlmap_remove(m, vlmap_version(m), key, strlen(key));

	// version is now at 2.
	vlmap_version_increment(m);

	uint8_t* value;
	int valuelength;

	// Get "foo" at version 2 -- this should be an error.
	int err = vlmap_get(m, vlmap_version(m), key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s\n", valuelength, value);
	} else {
		printf("`foo' is not present at version %lu.\n", vlmap_version(m));
	}

	err = vlmap_get(m, 0, key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s at version %d.\n", valuelength, value, 0);
	} else {
		printf("`foo' is not present at version %d.\n", 0);
	}

	err = vlmap_get(m, 1, key, keylength, &value, &valuelength);

	if(!err) {
		printf("foo => %.*s\n", valuelength, value);
	} else {
		printf("`foo' is not present at version %d.\n", 1);
	}

	//vlmap_destroy(m);

	exit(0);
}
