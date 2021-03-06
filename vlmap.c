#include "vlmap.h"

static inline int
random_level() {
	int i;
	int j = 0;
	for(i = 0; i < 20; i++)
		if(rand()%2) {
			j++;
		} else {
			break;
		}
	return j;
}

void
print_node(vlnode_t* n) {
	if(n != NULL)
		printf("[%.*s => %.*s] ", n->keylength, n->key, n->valuelength, n->value);
	else
		printf("NULL node\n");
}

static vlnode_t*
vlmap_real_remove_from_list(vlnode_t* root, vlnode_t* node, int level);

static vlnode_t*
vlmap_search_in_list(vlnode_t** rootptr, vlnode_t* node, int level);

vlmap*
vlmap_create() {
	vlmap* m = (vlmap*)calloc(1, sizeof(vlmap)+sizeof(vlnode_t*)*20);
	m->levels = 20;
	m->root = (vlnode_t**)(m+sizeof(vlmap)*20);
	m->version = 1;
	return m;
}

void
vlmap_destroy(vlmap* m) {
	if(m == NULL) return;

	int j;
	for(j = 0; j <= m->version; j++) {
		int i;
		for(i = m->levels-1; i >= 0; i--) {
			vlnode_t* cur = m->root[i];
			while(cur) {
				vlnode_t* next = cur->next[i];
				m->root[i] = vlmap_real_remove_from_list(m->root[i], cur, i);
				cur = next;
			}
		}
	}

	free(m);
}

void
vlnode_destroy(vlnode_t* n) {
	if(n == NULL) return;
	free(n);
}

static vlnode_t*
vlmap_create_node(uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength) {
	int level = random_level();
	vlnode_t* n = (vlnode_t*)calloc(1, sizeof(vlnode_t)+(level+1)*sizeof(vlnode_t*)+keylength+valuelength);
	n->keylength = keylength;
	n->valuelength = valuelength;
	n->created = version;
	n->level = level;

	n->next = (vlnode_t**)(n+sizeof(vlnode_t));
	n->key = (uint8_t*)(n+sizeof(vlnode_t)+(level+1)*sizeof(vlnode_t*));
	memcpy(n->key, key, keylength);

	if(value != NULL) {
		n->value = n->key+keylength;
		memcpy(n->value, value, valuelength);
	}
	return n;
}

static inline int
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

static inline int
vlmap_node_less_than(vlnode_t* a, vlnode_t* b) {
	return vlmap_compare_nodes(a, b) < 0;
}

static void
vlmap_insert_into_list(vlnode_t** rootptr, vlnode_t* node, int level) {
	if(level < 0) return;
	vlnode_t* root = rootptr[level];

	if(root == NULL) {
		if(node->level >= level)
			rootptr[level] = node;
		return vlmap_insert_into_list(rootptr, node, level-1);
	}

	if(vlmap_compare_nodes(node, root) < 0) {
		if(node->level >= level) {
			node->next[level] = rootptr[level];
			rootptr[level] = node;
		}
		return vlmap_insert_into_list(rootptr, node, level-1);
	}

	if(vlmap_compare_nodes(node, root) == 0) {
		root->removed = node->created;
	}

	if(vlmap_compare_nodes(node, root->next[level]) < 0) {
		if(node->level >= level) {
			node->next[level] = root->next[level];
			root->next[level] = node;
		}
		return vlmap_insert_into_list(rootptr, node, level-1);
	}

	return vlmap_insert_into_list(root->next, node, level);
}

static inline int
vlmap_vlnode_is_present(vlnode_t* n, uint64_t version) {
	return (n->created <= version &&
	(n->removed == 0 || n->removed > version));
}

static vlnode_t*
vlmap_search_in_list(vlnode_t** rootptr, vlnode_t* node, int level) {
	if(level < 0) {
		return rootptr[0];
	}

	vlnode_t* root = rootptr[level];

	if(root == NULL) {
		return vlmap_search_in_list(rootptr, node, level-1);
	}

	// node is smaller than root
	if(vlmap_compare_nodes(node, root) < 0) {
		if(level == 0)
			return NULL;
		return vlmap_search_in_list(rootptr, node, level-1);
	}

	// node is bigger than root, but smaller than
	// root's next
	if(vlmap_compare_nodes(node, root->next[level]) < 0) {
		return vlmap_search_in_list(rootptr, node, level-1);
	}
	return vlmap_search_in_list(root->next, node, level);
}

static vlnode_t*
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

void
vlmap_clean(vlmap* m, uint64_t version) {
	int i;
	for(i = m->levels-1; i >= 0; i--) {
		vlnode_t* cur = m->root[i];
		while(cur) {
			vlnode_t* next = cur->next[i];
			if(cur->removed > 0 && cur->removed < version) {
				m->root[i] = vlmap_real_remove_from_list(m->root[i], cur, i);
			}
			cur = next;
		}
	}

	m->oldest = version;
}

int
vlmap_insert(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength) {
	if(version >= m->version) {
		vlnode_t* node = vlmap_create_node(version, key, keylength, value, valuelength);
		vlmap_insert_into_list(m->root, node, m->levels-1);
		return 0;
	}

	return 1;
}

// This is a logical remove.
int
vlmap_remove(vlmap* m, uint64_t version, uint8_t* key, int keylength) {
	vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);

	int level = m->levels-1;
	vlnode_t* searched = NULL;
	if(vlmap_compare_nodes(node, m->root[0]) == 0)
		searched = m->root[0];
	else
		searched = vlmap_search_in_list(m->root, node, level);

	if(vlmap_compare_nodes(node, searched) != 0) {
		if(searched != NULL)
			searched = searched->next[0];
		else {
			vlnode_destroy(node);
			return 1;
		}
	}

	while(vlmap_compare_nodes(node, searched) == 0) {
		if(!vlmap_vlnode_is_present(searched, version)) {
			searched = searched->next[0];
		} else {
			searched->removed = version;
			vlnode_destroy(node);
			return 0;
		}
	}
	vlnode_destroy(node);
	return 1;
}

int
vlmap_get(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t** value, int* valuelength) {
	vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);
	int level = m->levels-1;
	vlnode_t* searched = NULL;
	if(vlmap_compare_nodes(node, m->root[0]) == 0)
		searched = m->root[0];
	else
		searched = vlmap_search_in_list(m->root, node, level);

	if(vlmap_compare_nodes(node, searched) != 0) {
		if(searched != NULL)
			searched = searched->next[0];
		else {
			vlnode_destroy(node);
			return 1;
		}
	}

	while(vlmap_compare_nodes(node, searched) == 0) {
		if(!vlmap_vlnode_is_present(searched, version)) {
			searched = searched->next[0];
		} else {
			*valuelength = searched->valuelength;
			*value = calloc(*valuelength, sizeof(uint8_t));
			memcpy(*value, searched->value, *valuelength);
			vlnode_destroy(node);
			return 0;
		}
	}
	vlnode_destroy(node);
	return 1;
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


vlmap_iterator*
vlmap_iterator_create(vlmap* m, uint64_t version, uint8_t* startkey, int startkeylen, uint8_t* endkey, int endkeylen) {
	vlmap_iterator* i = (vlmap_iterator*)calloc(1, sizeof(vlmap_iterator));
	i->version = version;
	i->startkey = startkey;
	i->startkeylen = startkeylen;
	i->endkey = endkey;
	i->endkeylen = endkeylen;

	vlnode_t* node = vlmap_create_node(version, startkey, startkeylen, NULL, 0);
	vlnode_t* searched = vlmap_search_in_list(m->root, node, m->levels-1);
	if(vlmap_compare_nodes(node, m->root[0]) == 0) {
		i->root = m->root[0];
		while(i->root) {
			if(vlmap_vlnode_is_present(i->root, i->version))
				break;
			i->root = i->root->next[0];
		}
		vlnode_destroy(node);
		if(i->root)
			return i;
		else {
			vlmap_iterator_destroy(i);
			return NULL;
		}
	}

	if(searched == NULL) {
		i->root = m->root[0];
		while(i->root) {
			if(vlmap_vlnode_is_present(i->root, i->version))
				break;
			i->root = i->root->next[0];
		}
		vlnode_destroy(node);
		if(i->root)
			return i;
		else {
			vlmap_iterator_destroy(i);
			return NULL;
		}
	}

	i->root = searched;
	if(i->root == NULL) {
		vlmap_iterator_destroy(i);
		vlnode_destroy(node);
		return NULL;
	}

	while(i->root) {
		if(vlmap_vlnode_is_present(i->root, i->version))
			break;
		i->root = i->root->next[0];
	}
	if(i->root)
		return i;
	else {
		vlmap_iterator_destroy(i);
		return NULL;
	}
	vlnode_destroy(node);
	return i;
}

void
vlmap_iterator_destroy(vlmap_iterator* i) {
	free(i);
}

int
vlmap_iterator_get_key(vlmap_iterator* i, uint8_t** key, int* keylength) {
	if(i->root != NULL) {
		*keylength = i->root->keylength;
		*key = calloc(*keylength, sizeof(uint8_t));
		memcpy(*key, i->root->key, *keylength);
		return 0;
	}
	return 1;
}

int
vlmap_iterator_get_value(vlmap_iterator* i, uint8_t** value, int* valuelength) {
	if(i->root != NULL) {
		*valuelength = i->root->valuelength;
		*value = calloc(*valuelength, sizeof(uint8_t));
		memcpy(*value, i->root->value, *valuelength);
		return 0;
	}
	return 1;
}

vlmap_iterator*
vlmap_iterator_next(vlmap_iterator* i) {
	vlnode_t* cur = i->root;

	cur = cur->next[0];
	while(cur) {
		if(vlmap_vlnode_is_present(cur, i->version))
			break;
		cur = cur->next[0];
	}

	vlnode_t* node = vlmap_create_node(i->version, i->endkey, i->endkeylen, NULL, 0);
	if(vlmap_compare_nodes(node, cur) <= 0) {
		vlnode_destroy(node);
		return NULL;
	}
	vlnode_destroy(node);
	i->root = cur;

	return i;
}

vlmap_iterator*
vlmap_iterator_remove(vlmap_iterator* i) {
	i->root->removed = i->version;
	return vlmap_iterator_next(i);
}
