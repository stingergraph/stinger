#ifndef  KV_STORE_H
#define  KV_STORE_H

#include <stdint.h>
#include <stdio.h>

#include "string/astring.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * MACROS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define KVSAFE(X) { \
  kv_return_t rtn = (X); if(rtn >= KV_RETURN_ERROR) {  \
    fprinf(stderr, "%s %d: %s\n", __func__, __LINE__, kv_error_string(rtn)); } 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * TYPEDEFS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
typedef enum kv_return kv_return_t;
typedef enum kv_type kv_type_t;

typedef struct kv_element kv_element_t;
typedef struct kv_list_element kv_list_element_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * ENUMS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define KV_RETURN_TYPES \
  TYPE(KV_SUCCESS), \
  TYPE(KV_RETURN_INFO), \
  TYPE(KV_EQUAL), \
  TYPE(KV_NOT_EQUAL), \
  TYPE(KV_DOES_NOT_EXIST), \
  TYPE(KV_RETURN_ERROR), \
  TYPE(KV_ALLOCATION_FAILED), \
  TYPE(KV_OUT_OF_BOUNDS), \
  TYPE(KV_RECEIVED_NULL), \
  TYPE(KV_PARSE_ERROR), \
  TYPE(KV_RETURN_MAX)

#if defined(TYPE)
#undef TYPE
#endif
#define TYPE(X) X
/** @brief the kv_return type used by all kv functions */
enum kv_return {
  KV_RETURN_TYPES
};
#undef TYPE

/** @brief the type of a given kv element */
enum kv_type {
  KV_NONE = 0,
  KV_I64,
  KV_DBL,
  KV_STR,
  KV_STR_STATIC,
  KV_KV,
  KV_LIST,
  KV_VECTOR,
  KV_PTR,
  KV_TRACKER
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * STRUCTS 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct kv_list_element {
  kv_element_t * data;
  kv_list_element_t * next; 
  kv_list_element_t * prev; 
};

/** @brief a single kv element */
struct kv_element {
  kv_type_t type;
  union {
    int64_t i64;
    double dbl;
    string_t str;
    void * ptr;

    struct {
      int64_t size;
      int64_t mask;
      int64_t elements;
      int64_t removed;
      kv_element_t ** keys;
      kv_element_t ** vals;
    } kv;

    struct {
      int64_t length;
      kv_list_element_t * head;
      kv_list_element_t * tail;
    } list;
    
    struct {
      int64_t length;
      int64_t max;
      kv_element_t ** arr;
    } vec;

    struct {
      kv_element_t * elements;
    } tracker;
  } data;
};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_ELEMENT FUNCTION PROTOTYPES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
kv_element_t *
new_kve_from_i64(int64_t i);

kv_element_t *
new_kve_from_dbl(double i);

kv_element_t *
new_kve_from_ptr(void * i);

kv_element_t *
new_kve_from_str(string_t * i);

kv_element_t *
new_kve_from_str_static(char * s);

kv_element_t
kve_from_i64(int64_t i);

kv_element_t
kve_from_dbl(double i);

kv_element_t
kve_from_ptr(void * i);

kv_element_t
kve_from_str(string_t * i);

kv_element_t
kve_from_str_static(char * i);

int64_t
kve_get_i64(kv_element_t * e);

double
kve_get_dbl(kv_element_t * e);

void *
kve_get_ptr(kv_element_t * e);

string_t *
kve_get_str(kv_element_t * e);

char *
kve_get_cstr(kv_element_t * e);

kv_return_t
kv_element_free(kv_element_t * element);

kv_return_t
kv_element_equal(kv_element_t * a, kv_element_t * b);

kv_return_t
kv_element_print(kv_element_t * a, int64_t);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_TRACKER FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
kv_return_t 
kv_tracker_new(kv_element_t ** track);

kv_return_t 
kv_tracker_init(kv_element_t * track);

kv_return_t 
kv_tracker_free(kv_element_t ** track);

kv_return_t 
kv_tracker_free_internal(kv_element_t * track);

kv_return_t 
kv_tracker_hash(kv_element_t * track, uint64_t * hash);

kv_return_t 
kv_tracker_is(kv_element_t * a, kv_element_t * b);

kv_return_t 
kv_tracker_append(kv_element_t * track, kv_element_t * val);

kv_element_t *
kv_tracker_i64(kv_element_t * track, int64_t i);

kv_element_t *
kv_tracker_dbl(kv_element_t * track, double i);

kv_element_t *
kv_tracker_ptr(kv_element_t * track, void * i);

kv_element_t *
kv_tracker_str(kv_element_t * track, string_t * i);

kv_element_t *
kv_tracker_str_static(kv_element_t * track, char * i);

kv_element_t *
kv_tracker_vector(kv_element_t * track);

kv_element_t *
kv_tracker_list(kv_element_t * track);

kv_element_t *
kv_tracker_store(kv_element_t * track);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_LIST FUNCTION PROTOTYPES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_list_new(kv_element_t ** rtn);

kv_return_t
kv_list_init(kv_element_t * list);

kv_return_t
kv_list_free_internal(kv_element_t * list);

kv_return_t
kv_list_free(kv_element_t ** list);

kv_return_t
kv_list_equal(kv_element_t * a, kv_element_t * b);

kv_return_t
kv_list_print(kv_element_t * list, int64_t indent);

kv_return_t
kv_list_hash(kv_element_t * list, uint64_t * hash);

kv_return_t
kv_list_length(kv_element_t * list, int64_t * length);

kv_return_t
kv_list_find(kv_element_t * list, kv_element_t * val, kv_list_element_t ** found);

kv_return_t
kv_list_insert_before(kv_element_t * list, kv_element_t * val, kv_list_element_t * found);

kv_return_t
kv_list_insert_after(kv_element_t * list, kv_element_t * val, kv_list_element_t * found);

kv_return_t
kv_list_insert_first(kv_element_t * list, kv_element_t * val);

kv_return_t
kv_list_insert_last(kv_element_t * list, kv_element_t * val);

kv_return_t
kv_list_pop_first(kv_element_t * list, kv_element_t ** out);

kv_return_t
kv_list_pop_last(kv_element_t * list, kv_element_t ** out);

kv_return_t
kv_list_remove(kv_element_t * list, kv_list_element_t * found);

kv_return_t
kv_list_first(kv_element_t * list, kv_list_element_t ** iterator, kv_element_t ** data);

kv_return_t
kv_list_next(kv_element_t * list, kv_list_element_t ** iterator, kv_element_t ** data);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_VECTOR FUNCTION PROTOTYPES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_vector_new_with_size(kv_element_t ** rtn, int64_t size);

kv_return_t
kv_vector_new(kv_element_t ** rtn);

kv_return_t
kv_vector_init_with_size(kv_element_t * rtn, int64_t size);

kv_return_t
kv_vector_init(kv_element_t * rtn);

kv_return_t
kv_vector_free_internal(kv_element_t * vec);

kv_return_t
kv_vector_free(kv_element_t ** vec);

kv_return_t
kv_vector_equal(kv_element_t * a, kv_element_t * b);

kv_return_t
kv_vector_hash(kv_element_t * vec, uint64_t * hash);

kv_return_t
kv_vector_get(kv_element_t * vec, int64_t index, kv_element_t ** out);

kv_return_t
kv_vector_set(kv_element_t * vec, int64_t index, kv_element_t * val);

kv_return_t
kv_vector_append(kv_element_t * vec, kv_element_t * val);

kv_return_t
kv_vector_resize(kv_element_t * vec, int64_t new_size);

kv_return_t
kv_vector_length(kv_element_t * vec, int64_t * length);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_STORE FUNCTION PROTOTYPES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_store_new(kv_element_t ** kv);

kv_return_t
kv_store_init(kv_element_t * kv);

kv_return_t
kv_store_free_internal(kv_element_t * kv);

kv_return_t
kv_store_free(kv_element_t ** kv);

kv_return_t
kv_store_equal(kv_element_t * a, kv_element_t * b);

kv_return_t
kv_store_print(kv_element_t * kv, int indent);

kv_return_t
kv_store_hash(kv_element_t * kv, uint64_t * hash);

kv_return_t
kv_store_get(kv_element_t * kv, kv_element_t * key, kv_element_t ** val);

kv_return_t
kv_store_set(kv_element_t * kv, kv_element_t * key, kv_element_t * val);

kv_return_t
kv_store_remove(kv_element_t * kv, kv_element_t * key);

kv_return_t
kv_store_contains(kv_element_t * kv, kv_element_t * key);

kv_return_t
kv_store_expand(kv_element_t * kv);

kv_return_t
kv_store_length(kv_element_t * kv, int64_t * out);

kv_return_t
kv_store_contains_value(kv_element_t * kv, kv_element_t * key);

kv_return_t
kv_store_first(kv_element_t * kv, kv_element_t *** ik, kv_element_t *** iv, kv_element_t ** key, kv_element_t ** val);

kv_return_t
kv_store_next(kv_element_t * kv, kv_element_t *** ik, kv_element_t *** iv, kv_element_t ** key, kv_element_t ** val);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ * 
 * OTHER FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const char * 
kv_error_string(kv_return_t rtn);

kv_return_t
kv_from_ini(FILE * ini, kv_element_t * tracker, kv_element_t ** top_level);

kv_return_t
kv_from_args(int argc, char ** argv, kv_element_t * tracker, kv_element_t ** top_level);

#endif  /*KV_STORE_H*/
