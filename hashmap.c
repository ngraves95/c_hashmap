#include "hashmap.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define LOAD_FACTOR 1.5		/* Reciprocal of actual load factor. */
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
 * Adds a hm_entry_t to the hashmap.
 * Verifies that entry != NULL.
 * Does not increment the count of entries.
 */
static void add_entry(struct hashmap *self, struct hm_entry_t *entry)
{
    if (!entry) {
	return;
    }

    int index = compress(self, self->hashcode(entry->key));
    // Add at head of linked list.
    entry->next = self->backing[index];
    self->backing[index] = entry;
}

/*
 * Resizes the backing array to the specified size and rehashes the entries.
 */
static int resize(struct hashmap *self, size_t new_size)
{
    struct hm_entry_t **resized = calloc(new_size, sizeof(*resized));
    if (!resized) {
	// Set error code.
	return 0;
    }

    // Rehash all entries in the backing array.
    struct hm_entry_t **old = self->backing;
    size_t old_size = self->size;

    self->size = new_size;
    self->backing = resized;

    // Rehash each bucket.
    size_t i;
    for (i = 0; i < old_size; ++i) {
	struct hm_entry_t *cur = old[i];
	// Rehash each item in the current bucket.
	while (cur) {
	    struct hm_entry_t *next = cur->next;
	    add_entry(self, cur);
	    cur = next;
	}
    }

    free(old);
    return 1;
}

/*
 * Frees all entries in the backing array, then frees the backing array.
 */
static void del_backing(struct hashmap *self)
{
    size_t i;
    for (i = 0; i < self->size; ++i) {
	struct hm_entry_t *cur = self->backing[i];
	while (cur) {
	    struct hm_entry_t *next = cur->next;
	    free(cur);
	    cur = next;
	}
    }

    free(self->backing);
}

/* === PUBLIC FUNCTIONS === */

struct hashmap *hashmap_init(
    struct hashmap *self,
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*)
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

    return self;
}

struct hashmap *hashmap_new(
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*)
)
{
    struct hashmap *hm = malloc(sizeof(*hm));

    if (!hm) {
	// Set error code.
	return NULL;
    }

    hashmap_init(hm, hash_func, equals_func);

    return hm;
}

void hashmap_del(struct hashmap *self)
{
    del_backing(self);
    free(self);
}

int hashmap_add(struct hashmap *self, void *key, void *value)
{
    if (!self) {
	// Set error code.
	return 0;
    }

    // Resize and rehash.
    if ((self->nentries * LOAD_FACTOR) >= self->size) {
	if (!resize(self, self->size * 2)) {
	    // Set error code.
	    return 0;
	}
    }

    int index = compress(self, self->hashcode(key));

    // Need to ensure uniqueness before adding.
    struct hm_entry_t *cur = self->backing[index];
    while (cur && !self->equals(cur->key, key)) {
	cur = cur->next;
    }

    // Indicates there is a duplicate entry.
    if (cur) {
	return 0;
    }

    struct hm_entry_t *pair = malloc(sizeof(*pair));
    if (!pair) {
	// Set error code.
	return 0;
    }

    pair->key = key;
    pair->value = value;

    add_entry(self, pair);

    ++self->nentries;

    return 1;
}

void *hashmap_remove(struct hashmap *self, void *key)
{
    if (!self) {
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

void hashmap_clear(struct hashmap *self)
{
    del_backing(self);
    hashmap_init(self, self->hashcode, self->equals);
}

size_t hashmap_size(struct hashmap *self)
{
    return self->nentries;
}

/* === Public Utilities === */

int hashmap_str_hash(void *key)
{
    int hash = 0;
    int exp = 1;
    char *str = key;

    while (*str) {
	char cur = *str;
	hash += ((int) cur) * exp;
	++str;
	exp = (exp << 5) - exp; // *= 31.
    }

    return hash;
}

int hashmap_str_eq(void *str1, void *str2)
{
    // strcmp returns 0 if the string are equal.
    return !strcmp(str1, str2);
}
