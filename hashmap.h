#ifndef ___HASHMAP
#define ___HASHMAP

/*
 * Forward declaration of the struct. User cannot deference the hashmap directly.
 */
struct hashmap;

/*
 * Takes in a function void* -> int for the hashcode, a function
 * void*,void* -> int to test for equality, and a void* -> void function
 * to free the value associated with a key.
 *
 * If equals_func == NULL, default to reference equality.
 * If hash_func == NULL, default to address of the key.
 * If free_func == NULL, no freeing is done.
 */
struct hashmap *hashmap_init(
    struct hashmap *self,
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*),
    void(*free_func)(void*)
);

/*
 * Allocates memory for a new hashmap and initializes it.
 * Returns a pointer to that hashmap.
 */
struct hashmap *hashmap_new(
    int(*hash_func)(void*),
    int(*equals_func)(void*, void*),
    void(*free_func)(void*)
);

/*
 * Adds a (key, value) pair to the hashmap.
 * Returns TRUE if successful, FALSE otherwise.
 */
int hashmap_add(struct hashmap *hm, void *key, void *value);

/*
 * Removes the (key, value) pair from the hashmap.
 * Returns the value associated with the key.
 * If the key does not exist in the hashmap, returns NULL and ENOKEY is set.
 */
void *hashmap_remove(struct hashmap *hm, void *key);

/*
 * Returns the value associated with key. If the key does not exist in
 * the hashmap, returns NULL and ENOKEY is set.
 */
void *hashmap_get(struct hashmap* hm, void *key);

/*
 * Returns TRUE if the key exists in the hashmap, FALSE otherwise.
 */
int hashmap_contains(struct hashmap *hm, void *key);

#endif
