#include "hashmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST_PASS() fprintf(stderr, "[PASS] %s\n", __func__)
#define TEST_FAIL(line) fprintf(stderr, "[FAILURE] %s failed at line: %d\n", __func__, line)
#define FAILURE_PRINT(exp, act) fprintf(stderr, "\t  Expected: <%p>. Actual: <%p>.\n", (void *)(long)(exp), (void *)(long)(act))


#define __assert(cond, line)			\
  if (!(cond)) {				\
      TEST_FAIL(line);				\
      return;					\
  }

#define __assert_eq(exp, act, line)		\
  if ((exp) != (act)) {				\
      TEST_FAIL(line);				\
      FAILURE_PRINT((exp), (act));		\
      return;					\
  }

#define __assert_neq(exp, act, line)		\
  if ((exp) == (act)) {				\
      TEST_FAIL(line);				\
      FAILURE_PRINT((exp), (act));		\
      return;					\
  }

#define assert(cond) __assert(cond, __LINE__)
#define assert_equal(a, b) __assert_eq((a), (b), __LINE__)
#define assert_not_equal(a, b) __assert_neq((a), (b), __LINE__)

#define delimiter() ","
#define log_name() __log
#define init_log(filename) FILE * log_name() = fopen(filename, "a");
#define log(func_with_args)						\
    do {								\
	clock_t start = clock();					\
	func_with_args;							\
	clock_t stop = clock();						\
	fprintf(log_name(), "%lu%s", stop - start, delimiter());	\
    } while (0);

#define close_log(filename)						\
    do {								\
        fprintf(log_name(), "-1\n");					\
	fclose(log_name());						\
    } while (0);

/*
 * Tests adding 2 non-colliding keys, then tests colliding keys.
 */
void test_add_int(struct hashmap *hm)
{
    hashmap_clear(hm);

    int key1 = 10;
    int key2 = 12;
    int collide_key1 = 20;

    int value1 = 0xCAFE;
    int value2 = 0xBABE;
    int value_collide_key1 = 0xDEAD;

    hashmap_add(hm, (void*)(long)key1, (void*)(long)value1);
    hashmap_add(hm, (void*)(long)key2, (void*)(long)value2);
    hashmap_add(hm, (void*)(long)collide_key1, (void*)(long)value_collide_key1);

    int get1_key_lit = (long)hashmap_get(hm, as_key(10));
    int get2_key_lit = (long)hashmap_get(hm, as_key(12));
    int get_collide_lit = (long)hashmap_get(hm, as_key(20));

    int get1_key_ref = (long)hashmap_get(hm, as_key(key1));
    int get2_key_ref = (long)hashmap_get(hm, as_key(key2));
    int get_collide_ref = (long)hashmap_get(hm, as_key(collide_key1));

    assert_equal(value1, get1_key_lit);
    assert_equal(value1, get1_key_ref);

    assert_equal(get_collide_lit, value_collide_key1);
    assert_equal(get_collide_ref, value_collide_key1);

    assert_equal(value2, get2_key_lit);
    assert_equal(value2, get2_key_ref);

    TEST_PASS();
}

/*
 * Tests adding enough elements to resize the hashmap.
 */
void test_resize(struct hashmap *hm)
{
    hashmap_clear(hm);

    size_t num_items = 10000;

    // Add items where key == value.
    unsigned long i;
    for (i = 0; i < num_items; ++i) {
	hashmap_add(hm, (void *) i, (void *) i);
    }

    assert_equal(num_items, hashmap_size(hm));

    // Verify hashmap returns the correct mappings.
    for (i = 0; i < num_items; ++i) {
	assert_equal(i, (unsigned long)hashmap_get(hm, (void *) i));
    }

    assert_equal(num_items, hashmap_size(hm));
    TEST_PASS();
}

/*
 * Hash function that will always produce a collision.
 */
static int collision_hash(void *key)
{
    return 1;
}

void test_collision(void)
{
    struct hashmap *hm = hashmap_new(collision_hash, NULL);
    size_t num_items = 1024;

    // Add items where key == value.
    unsigned long i;
    for (i = 0; i < num_items; ++i) {
	assert(hashmap_add(hm, (void *) i, (void *) i));
    }

    assert_equal(num_items, hashmap_size(hm));

    // Verify hashmap returns the correct mappings.
    for (i = 0; i < num_items; ++i) {
	assert_equal(i, (unsigned long)hashmap_get(hm, (void *) i));
    }

    assert_equal(num_items, hashmap_size(hm));

    hashmap_del(hm);
    TEST_PASS();
}

static void test_str_base(struct hashmap *hm)
{
    char *str_key1 = "This is a string key";
    char *str_key2 = "This is another string key";
    char *str_key3 = str_key1;

    char *str_val1 = "Value1";
    char *str_val2 = "Value2";
    char str_val3[] = "Value2";


    assert_not_equal(str_val2,  str_val3);
    assert(hashmap_add(hm, str_key1, str_val1));

    assert(hashmap_add(hm, str_key2, str_val2));
    assert_equal(0, hashmap_add(hm, str_key3, str_val2)); // this should fail.

    assert_equal(str_val1, hashmap_get(hm, str_key1));
    assert_equal(str_val2, hashmap_get(hm, str_key2));
}


void test_add_str_with_good_hash_good_eq(void)
{

    struct hashmap *hm = hashmap_new(hashmap_str_hash, hashmap_str_eq);
    test_str_base(hm);
    hashmap_del(hm);
    TEST_PASS();
}

void test_add_str_basic_hash_basic_eq(void)
{
    struct hashmap *hm = hashmap_new(NULL, NULL);
    test_str_base(hm);
    hashmap_del(hm);
    TEST_PASS();
}



void test_add_immediate_remove(struct hashmap *hm)
{
    hashmap_clear(hm);

    // Adds an item to the hashmap, then immediately removes it.
    int i, num_items = 1000000;
    for (i = 0; i < num_items; ++i)
    {
	assert(hashmap_add(hm, as_key(i), as_key(i)));
	assert_equal(1, hashmap_size(hm));
	assert_equal(i, (int)(long)hashmap_remove(hm, as_key(i)));
	assert_equal(0, hashmap_size(hm));
    }

    assert_equal(0, (int)(long)hashmap_size(hm));

    TEST_PASS();
}

int main(void)
{
    // Logging variables.
    init_log("hashmap_performance.log");

    struct hashmap *hm = hashmap_new(NULL, NULL);

    log(test_add_int(hm));
    log(test_resize(hm));
    log(test_add_str_with_good_hash_good_eq());
    log(test_add_str_basic_hash_basic_eq());
    log(test_collision());

    int i;
    for (i = 0; i < 10; i++) {
	log(test_add_immediate_remove(hm));
    }

    close_log();

    hashmap_del(hm);
    return 0;
}
