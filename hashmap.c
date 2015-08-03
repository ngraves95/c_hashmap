#include "hashmap.h"
#include <stdlib.h>
#define COMPRESS(hm, hash) (hash % hm->size)

#define HASHMAP_INIT_SIZE (10)	/* Default size of a hashmap. */


struct hm_entry_t {
    struct hm_entry_t *next;
    void *key;
    void *value;
};

struct hashmap {
    size_t size;                        /* Size of backing array. */
    size_t nentries;                    /* Number of entries in backing array. */
    struct hm_entry_t **backing;	/* The backing array of pointers to entries. */
    int (*hashcode)(void*);		/* Hash function. */
    int (*equals)(void*, void*);        /* Equals function. */
    void(*del)(void*);		        /* Free function; deletes an element. */
};

/* === PRIVATE FUNCTIONS === */

/*
 * Compressed a hash value to fit the table. Inline for efficiency.
 */
static inline int compress(struct hashmap *hm, int hash)
{
    return hash % hm->size;
}

/*
 * Simple reference equality.
 */
static int ref_eq(void *a, void *b)
{
    return a == b;
}

/*
 * Use address as hash. This is a bad hash.
 */
static int addr_hash(void *key)
{
    return (int)((unsigned long) key);
}

/*
 * "free" function that does nothing.
 */
static void mock_free(void *param)
{
    return;
}

/* === PUBLIC FUNCTIONS === */

struct hashmap *hashmap_init(
    struct hashmap *self,
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*),
    void(*free_func)(void*)
    )
{
    struct hm_entry_t **arr = calloc(HASHMAP_INIT_SIZE, sizeof(*arr));
    if (!arr) {
	// Set error code
	return NULL;
    }

    self->size = HASHMAP_INIT_SIZE;
    self->nentries = 0;
    self->backing = arr;
    self->hashcode = (hash_func) ? hash_func : addr_hash;
    self->equals = (equals_func) ? equals_func : ref_eq;
    self->del = (free_func) ? free_func : mock_free;

    return self;
}

struct hashmap *hashmap_new(
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*),
    void(*free_func)(void*)
)
{
    struct hashmap *hm = malloc(sizeof(*hm));

    if (!hm) {
	// Set error code.
	return NULL;
    }

    hashmap_init(hm, hash_func, equals_func, free_func);

    return hm;
}

int hashmap_add(struct hashmap *self, void *key, void *value)
{
    if (!self || !key || !value) {
	// Set error code.
	return 0;
    }

    // Consider resizing and rehashing.

    int index = compress(self, self->hashcode(key));
    struct hm_entry_t *pair = malloc(sizeof(*pair));
    if (!pair) {
	// Set error code/
	return 0;
    }

    // Add at head of linked list.
    pair->next = self->backing[index];
    self->backing[index] = pair;

    ++self->nentries;

    return 1;
}

void *hashmap_remove(struct hashmap *self, void *key)
{
    if (!self || !key) {
	// Set error code.
	return NULL;
    }

    int index = compress(self, self->hashcode(key));

    struct hm_entry_t **cur = &(self->backing[index]);

    while (*cur && !self->equals((*cur)->key, key)) {
	cur = &((*cur)->next);
    }

    // Item not in list. Return NULL and set error code.
    if (!*cur) {
	// Set error code.
	return NULL;
    }

    // Remove item from the list.
    struct hm_entry_t *removed = *cur;
    void *retval = removed->value;

    *cur = (*cur)->next;
    --self->nentries;
    free(removed);

    // Consider rehashing if size:nentries ratio falls below a certain point.

    return retval;
}

void *hashmap_get(struct hashmap *self, void *key)
{
    int i = compress(self, self->hashcode(key));
    struct hm_entry_t *cur = self->backing[i];

    while (cur && !self->equals(cur->key, key)) {
	cur = cur->next;
    }

    return (cur) ? cur->value : NULL;
}

int hashmap_contains(struct hashmap *self, void *key)
{
    return hashmap_get(self, key) != NULL;
}
