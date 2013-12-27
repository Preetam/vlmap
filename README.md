vlmap
=====
vlmap is a versioned, ordered map built on a skip list.

Keys and values are `uint8_t`s, and the keys are sorted in lexicographical order.

Since it is versioned, snapshot reads are possible.

This is **not** guaranteed to be thread-safe, so it would be best to do writes sequentially.
You should be able to do concurrent reads as long as the versions being used aren't cleaned.

License
----
MIT

API
---
Here's a copy of the header file:

```c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef VLMAP_H
#define VLMAP_H

struct vlmap_node {
	uint8_t* key;
	int keylength;
	uint8_t* value;
	int valuelength;

	uint64_t created;
	uint64_t removed;

	int level;

	struct vlmap_node** next;
};

typedef struct vlmap_node vlnode_t;

typedef struct {
	uint64_t version;
	uint64_t oldest;
	int levels;
	vlnode_t** root;
} vlmap;

typedef struct {
	uint64_t version;
	vlnode_t* root;
	uint8_t* startkey;
	int startkeylen;
	uint8_t* endkey;
	int endkeylen;
} vlmap_iterator;


// vlmap_create returns a pointer to a new vlmap.
vlmap*
vlmap_create();

// vlmap_destroy destroys a vlmap and all of
// its nodes.
void
vlmap_destroy(vlmap* m);

// vlmap_insert inserts a key-value pair into the map at a certain version.
int
vlmap_insert(vlmap* m, uint64_t version, uint8_t* key, int keylength,
	uint8_t* value, int valuelength);

// vlmap_remove logically removes a node from the map, if it matches
// the given key, at a certain version. Returns 0 on success.
int
vlmap_remove(vlmap* m, uint64_t version, uint8_t* key, int keylength);

// vlmap_get gets a key-value at a certain version.
// On success, 0 is returned and value and valuelength are filled
// with the respective data.
// **It's the caller's responsibility to free the value**.
// On error, value and valuelength will not be modified.
int
vlmap_get(vlmap* m, uint64_t version, uint8_t* key, int keylength,
	uint8_t** value, int* valuelength);

// vlmap_version returns the current version of the map.
uint64_t
vlmap_version(vlmap* m);

// vlmap_version_increment increments the current version.
void
vlmap_version_increment(vlmap* m);

void
vlmap_print(vlmap* m, int version);

// vlmap_clean removes any node in the map
// older than version.
void
vlmap_clean(vlmap* m, uint64_t version);

// vlmap_iterator_create creates a new iterator,
// which must later be destroyed with vlmap_iterator_destroy,
// at a version. This allows for snapshot range reads.
//
// A start key and an end key are required, but one can
// use `\x00' and `\xff' as the start and end, respectively,
// to essentially read from the entire range.
vlmap_iterator*
vlmap_iterator_create(vlmap* m, uint64_t version,
	uint8_t* startkey, int startkeylen,
	uint8_t* endkey, int endkeylen);

// vlmap_iterator_destroy frees the space allocated
// to a vlmap_iterator.
void
vlmap_iterator_destroy(vlmap_iterator* i);

// vlmap_iterator_get_value works the same way as calling
// vlmap_get on the current node of the iterator.
// This returns 0 on success, and value must be freed by
// the caller.
int
vlmap_iterator_get_value(vlmap_iterator* i, uint8_t** value, int* valuelength);

// vlmap_iterator_next moves to the next node in the map.
// Map elements not in the current snapshot are ignored.
// Returns NULL when out of range, so it's important to
// store the original pointer to the iterator to free it
// using vlmap_iterator_destroy.
vlmap_iterator*
vlmap_iterator_next(vlmap_iterator* i);

#endif
```