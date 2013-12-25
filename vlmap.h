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
	int levels;
	vlnode_t** root;
} vlmap;

vlmap*
vlmap_create();

void
vlmap_destroy(vlmap* m);

int
vlmap_insert(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength);

int
vlmap_remove(vlmap* m, uint64_t version, uint8_t* key, int keylength);

int
vlmap_get(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t** value, int* valuelength);

uint64_t
vlmap_version(vlmap* m);

void
vlmap_version_increment(vlmap* m);

#endif
