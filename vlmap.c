#include "vlmap.h"

int
random_level() {
	int i;
	int j = 0;
	for(i = 0; i < 5; i++)
		if(rand()%4 == 0)
			j++;
	return j;
}

void
print_node(vlnode_t* n) {
	printf("[%.*s => %.*s] ", n->keylength, n->key, n->valuelength, n->value);
}

vlnode_t*
vlmap_real_remove_from_list(vlnode_t* root, vlnode_t* node, int level);

vlnode_t*
vlmap_search_in_list(vlnode_t* root, vlnode_t* node, int level);

vlmap*
vlmap_create() {
	vlmap* m = (vlmap*)calloc(1, sizeof(vlmap));
	m->levels = 5;
	m->root = (vlnode_t**)calloc(m->levels, sizeof(vlnode_t*));
}

void
vlmap_destroy(vlmap* m) {
	int i;
	for(i = m->levels-1; i >= 0; i--) {
		while(m->root[i]) {
			m->root[i] = vlmap_real_remove_from_list(m->root[i], m->root[i], i);
		}
	}
	free(m->root);
	free(m);
}

void
vlnode_destroy(vlnode_t* n) {
	if(n == NULL) {
		return;
	}
	free(n->next);
	free(n);
}

vlnode_t*
vlmap_create_node(uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength) {
	vlnode_t* n = (vlnode_t*)calloc(1, sizeof(vlnode_t));
	n->key = key;
	n->keylength = keylength;
	n->value = value;
	n->valuelength = valuelength;
	n->created = version;
	n->level = random_level();
	n->next = (vlnode_t**)calloc(n->level+1, sizeof(vlnode_t*));
	return n;
}

int
vlmap_compare_nodes(vlnode_t* a, vlnode_t* b) {
	if(b == NULL) {
		return -1;
	}
	int cmp = memcmp(a->key, b->key, a->keylength <= b->keylength ? a->keylength : b->keylength);
	if(cmp == 0) {
		if(a->keylength == b->keylength) {
			return 0;
		}
		if(a->keylength < b->keylength) {
			return -1;
		}

		return 1;
	}

	return cmp;
}

int
vlmap_node_less_than(vlnode_t* a, vlnode_t* b) {
	return vlmap_compare_nodes(a, b) < 0;
}

vlnode_t*
vlmap_insert_into_list(vlnode_t* root, vlnode_t* node, int level) {
	if(root == NULL) {
		return node;
	}

	// This is strictly less than
	if(vlmap_node_less_than(node, root)) {
		node->next[level] = root;
		return node;
	}

	// The key already exists, so logically remove
	// the old node and insert the new node in
	// front of it.
	if(vlmap_compare_nodes(node, root) == 0) {
		root->removed = node->created;
		node->next[level] = root;
		return node;
	}

	root->next[level] = vlmap_insert_into_list(root->next[level], node, level);
	return root;
}

int
vlmap_vlnode_is_present(vlnode_t* n, uint64_t version) {
	return (n->created <= version &&
	(n->removed == 0 || n->removed > version));
}

vlnode_t*
vlmap_search_in_list(vlnode_t* root, vlnode_t* node, int level) {
	if(root == NULL) {
		return NULL;
	}

	if(vlmap_compare_nodes(node, root) == 0) {
		if(vlmap_vlnode_is_present(root, node->created)) {
			return root;
		}
		return NULL;
	}

	if(vlmap_compare_nodes(node, root->next[level]) < 0 && level != 0) {
		return vlmap_search_in_list(root, node, level-1);
	}

	return vlmap_search_in_list(root->next[level], node, level);
}

vlnode_t*
vlmap_real_remove_from_list(vlnode_t* root, vlnode_t* node, int level) {
	if(root == NULL) {
		return NULL;
	}

	if(vlmap_compare_nodes(node, root) == 0) {		
		vlnode_t* next = root->next[level];
		if(level == 0) {
			if(node != root) {
				vlnode_destroy(node);
			}
			vlnode_destroy(root);
		}
		return next;
	}

	root->next[level] = vlmap_real_remove_from_list(root->next[level], node, level);
	return root;
}

int
vlmap_insert(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength) {
	if(version >= m->version) {
		vlnode_t* node = vlmap_create_node(version, key, keylength, value, valuelength);

		int i;
		for(i = 0; i <= node->level; i++) {
			m->root[i] = vlmap_insert_into_list(m->root[i], node, i);
		}

		return 0;
	}

	return 1;
}

// This is a logical remove.
int
vlmap_remove(vlmap* m, uint64_t version, uint8_t* key, int keylength) {
	if(version >= m->version) {
		vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);
		int level = m->levels-1;
		vlnode_t* searched = vlmap_search_in_list(m->root[level], node, level);
		while(searched == NULL) {
			if(level == 0) {
				vlnode_destroy(node);
				return 1;
			}
			level--;
			searched = vlmap_search_in_list(m->root[level], node, level);
		}
		vlnode_destroy(node);
		searched->removed = version;
		return 0;
	}

	return 1;
}

int
vlmap_get(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t** value, int* valuelength) {
	vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);

	int level = m->levels-1;
	vlnode_t* searched = vlmap_search_in_list(m->root[level], node, level);
	while(searched == NULL) {
		if(level == 0) {
			vlnode_destroy(node);
			return 1;
		}
		level--;
		searched = vlmap_search_in_list(m->root[level], node, level);
	}

	*valuelength = searched->valuelength;
	*value = calloc(*valuelength, sizeof(uint8_t));
	memcpy(*value, searched->value, *valuelength);
	vlnode_destroy(node);
	return 0;
}

uint64_t
vlmap_version(vlmap* m) {
	return m->version;
}

void
vlmap_version_increment(vlmap* m) {
	m->version++;
}

void
vlmap_print(vlmap* m, int version) {
	int i;
	printf("Version: %d\n================", version);
	for(i = 0; i < m->levels; i++) {
		printf("\nLevel %d:\n--------\n", i);
		vlnode_t* cur = m->root[i];
		while(cur) {
			if(vlmap_vlnode_is_present(cur, version))
				print_node(cur);
			cur = cur->next[i];
		}
	}
}
