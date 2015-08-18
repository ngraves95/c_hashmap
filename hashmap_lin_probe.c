#include "hashmap.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define LOAD_FACTOR 1.5		/* Reciprocal of actual load factor. */
#define HASHMAP_INIT_SIZE (10)	/* Default size of a hashmap. */

typedef enum entry_state {
    EMPTY = 0,
    DELETED,
    ACTIVE
} entry_state_t;

struct hm_entry_t {
    void *key;
    void *value;
    entry_state_t state;
};

struct hashmap {
    size_t size;                        /* Size of backing array. */
    size_t nentries;                    /* Number of entries in backing array. */
    struct hm_entry_t *backing; 	/* The backing array of pointers to entries. */
    int (*hashcode)(void*);		/* Hash function. */
    int (*equals)(void*, void*);        /* Equals function. */
};


/* === PRIVATE FUNCTIONS === */

/*
 * Compresses a hash value to fit the table. Inline for efficiency.
 */
static inline int compress(struct hashmap *hm, int hash)
{
    return hash % hm->size;
}

/*
 * Simple reference equality.
 */
static inline int ref_eq(void *a, void *b)
{
    return a == b;
}

/*
 * Use address as hash. This is a bad hash.
 */
static inline int addr_hash(void *key)
{
    return (int)((unsigned long) key);
}


static inline int update_index(struct hashmap *hm, int index)
{
    return (index + 1) % hm->size;
}

/*
 * Adds a hm_entry_t to the hashmap.
 * Verifies that entry != NULL.
 * Does not increment the count of entries.
 */
static int add_entry(struct hashmap *self, struct hm_entry_t *entry)
{
    int index = compress(self, self->hashcode(entry->key));
    int index_cycle_flag = index;
    struct hm_entry_t cur = self->backing[index];

    while (cur.state == ACTIVE) {
	// Moving this inside the while-loop allows for no post
	// loop condition checking, improving performance by ~5%.
	if (self->equals(cur.key, entry->key)) {
	    return 0;
	}

	index = update_index(self, index);
	cur = self->backing[index];

	if (index == index_cycle_flag) {
	    return 0;
	}
    }

    self->backing[index] = *entry;
    cur.state = ACTIVE;

    return 1;
}

/*
 * Resizes the backing array to the specified size and rehashes the entries.
 */
static int resize(struct hashmap *self, size_t new_size)
{
    struct hm_entry_t *resized = calloc(new_size, sizeof(*resized));
    if (!resized) {
	// Set error code.
	return 0;
    }

    // Rehash all entries in the backing array.
    struct hm_entry_t *old = self->backing;
    size_t old_size = self->size;

    self->size = new_size;
    self->backing = resized;

    // Rehash each bucket.
    size_t i;
    for (i = 0; i < old_size; ++i) {
	struct hm_entry_t cur = old[i];
	if (cur.state == ACTIVE) {
	    add_entry(self, &cur);
	}
    }

    free(old);
    return 1;
}

/*
 * Frees all entries in the backing array, then frees the backing array.
 */
static inline void del_backing(struct hashmap *self)
{
    free(self->backing);
}

/*
 * Finds and returns the matching entry.
 * Returns NULL if no matches.
 */
static struct hm_entry_t *find_entry(struct hashmap *self, void *key)
{
    int i = compress(self, self->hashcode(key));
    int index_cycle_flag = i;
    struct hm_entry_t cur = self->backing[i];

    // Should stop when: finds empty, or find active && matching
    while (cur.state != ACTIVE || !self->equals(cur.key, key)) {
	if (cur.state == EMPTY) {
	    return NULL;
	}

	i = update_index(self, i);
	cur = self->backing[i];

	if (i == index_cycle_flag) {
	    return NULL;
	}
    }

    return &self->backing[i];
}

/* === PUBLIC FUNCTIONS === */

struct hashmap *hashmap_init(
    struct hashmap *self,
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*)
    )
{
    struct hm_entry_t *arr = calloc(HASHMAP_INIT_SIZE, sizeof(*arr));
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
    // Resize and rehash.
    if ((self->nentries * LOAD_FACTOR) >= self->size) {
	if (!resize(self, self->size * 2)) {
	    // Set error code.
	    return 0;
	}
    }

    struct hm_entry_t pair;
    pair.key = key;
    pair.value = value;
    pair.state = ACTIVE;

    if (!add_entry(self, &pair)) {
	// Set error code.
	return 0;
    }

    ++self->nentries;

    return 1;
}

void *hashmap_remove(struct hashmap *self, void *key)
{
    struct hm_entry_t *cur = find_entry(self, key);

    if (!cur || !cur->state) {
	return NULL;
    }

    void *retval = cur->value;
    // Set flag to deleted.
    cur->state = DELETED;
    --self->nentries;

    // Consider rehashing if size:nentries ratio falls below a certain point.

    return retval;
}

void *hashmap_get(struct hashmap *self, void *key)
{
    struct hm_entry_t *cur = find_entry(self, key);
    return (cur && cur->state == ACTIVE) ? cur->value : NULL;
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

/* === Public Utilities (not methods!) === */

int hashmap_str_hash(void *key)
{
    if (!key) {
	return 0;
    }

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
    if (str1 == str2) {
	return 1;
    }

    if (!str1 || !str2) {
	return 0;
    }

    // strcmp returns 0 if the string are equal.
    return !strcmp(str1, str2);
}
