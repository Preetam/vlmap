#include "vlmap.h"

vlnode_t*
vlmap_real_remove_from_list(vlnode_t* root, vlnode_t* node);

vlmap*
vlmap_create() {
	vlmap* m = (vlmap*)calloc(1, sizeof(vlmap));
}

void
vlmap_destroy(vlmap* m) {
	while(m->root) {
		m->root = vlmap_real_remove_from_list(m->root, m->root);
	}

	free(m);
}

void
vlnode_destroy(vlnode_t* n) {
	if(n == NULL) {
		return;
	}

	//if(n->key != NULL)
	//	free(n->key);
	//if(n->value != NULL)
	//	free(n->value);

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
	return n;
}

int
vlmap_compare_nodes(vlnode_t* a, vlnode_t* b) {
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
vlmap_insert_into_list(vlnode_t* root, vlnode_t* node) {
	if(root == NULL) {
		return node;
	}

	// This is strictly less than
	if(vlmap_node_less_than(node, root)) {
		node->next = root;
		return node;
	}

	// The key already exists, so logically remove
	// the old node and insert the new node in
	// front of it.
	if(vlmap_compare_nodes(node, root) == 0) {
		root->removed = node->created;
		node->next = root;
		return node;
	}

	root->next = vlmap_insert_into_list(root->next, node);
	return root;
}

int
vlmap_vlnode_is_present(vlnode_t* n, uint64_t version) {
	return (n->created <= version &&
	(n->removed == 0 || n->removed > version));
}

void
vlmap_logical_remove_from_list(vlnode_t* root, vlnode_t* node) {
	if(root == NULL) {
		vlnode_destroy(node);
		return;
	}

	if(vlmap_compare_nodes(node, root) == 0) {

		// Only "remove" it if it's not already removed
		if(vlmap_vlnode_is_present(root, node->created))
			root->removed = node->created;

		vlnode_destroy(node);
		return;
	}

	return vlmap_logical_remove_from_list(root->next, node);
}

vlnode_t*
vlmap_search_in_list(vlnode_t* root, vlnode_t* node) {
	if(root == NULL) {
		vlnode_destroy(node);
		return NULL;
	}

	if(vlmap_compare_nodes(node, root) == 0) {

		if(vlmap_vlnode_is_present(root, node->created)) {
			vlnode_destroy(node);
			return root;
		}

		vlnode_destroy(node);
		return NULL;
	}

	return vlmap_search_in_list(root->next, node);
}

vlnode_t*
vlmap_real_remove_from_list(vlnode_t* root, vlnode_t* node) {
	if(root == NULL) {
		if(node != NULL)
			vlnode_destroy(node);
		return NULL;
	}

	if(vlmap_compare_nodes(node, root) == 0) {		
		vlnode_t* next = root->next;
		vlnode_destroy(node);
		if(node != root)
			vlnode_destroy(root);
		return next;
	}

	root->next = vlmap_real_remove_from_list(root->next, node);
	return root;
}

int
vlmap_insert(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t* value, int valuelength) {
	if(version >= m->version) {
		vlnode_t* node = vlmap_create_node(version, key, keylength, value, valuelength);
		m->root = vlmap_insert_into_list(m->root, node);
		return 0;
	}

	return 1;
}

// This is a logical remove.
int
vlmap_remove(vlmap* m, uint64_t version, uint8_t* key, int keylength) {
	if(version >= m->version) {
		vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);
		vlmap_logical_remove_from_list(m->root, node);
		return 0;
	}

	return 1;
}

int
vlmap_get(vlmap* m, uint64_t version, uint8_t* key, int keylength, uint8_t** value, int* valuelength) {
	vlnode_t* node = vlmap_create_node(version, key, keylength, NULL, 0);
	vlnode_t* searched = vlmap_search_in_list(m->root, node);
	if(searched == NULL) {
		return 1;
	}

	memcpy(value, &searched->value, searched->valuelength);
	*valuelength = searched->valuelength;
	
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

