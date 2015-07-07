#include "kv_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if SAFETY_ON
#define SAFETY(X) X
#define IFNULL(X, Y) if(NULL == (X)) { Y; }
#else
#define SAFETY(X) 
#define IFNULL(X, Y) 
#endif

#define KV_VECTOR_DEFAULT_SIZE 32
#define KV_VECTOR_GROW_RATE 2

#define KV_STORE_DEFAULT_SIZE 32
#define KV_STORE_GROW_RATE 2
#define KV_STORE_THRESHOLD 0.75

#define KV_DELETED_PTR ((kv_element_t *)(INT64_MAX))
#define KV_NONE_PTR    ((kv_element_t *)(NULL))

/* two spaces */
#define KV_INDENT "  "

#define FATAL(X, ...) fprintf(stderr, "FATAL (%s %d): " X "\n", __func__, __LINE__, __VA_ARGS__); abort();
#define ERROR(X, ...) fprintf(stderr, "ERROR (%s %d): " X "\n", __func__, __LINE__, __VA_ARGS__);
#define WARNING(X, ...) fprintf(stderr, "WARNING (%s %d): " X "\n", __func__, __LINE__, __VA_ARGS__);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * STATIC LOCAL FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int64_t
mix(int64_t n) {
  n = (~n) + (n << 21); // n = (n << 21) - n - 1;
  n = n ^ (n >> 24);
  n = (n + (n << 3)) + (n << 8); // n * 265
  n = n ^ (n >> 14);
  n = (n + (n << 2)) + (n << 4); // n * 21
  n = n ^ (n >> 28);
  n = n + (n << 31);
  return n;
}

static void
print_indent(int64_t indent) {
  while(indent--) {
    printf(KV_INDENT);
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_ELEMENT FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_element_t *
new_kve_from_i64(int64_t i) {
  kv_element_t * rtn = malloc(sizeof(kv_element_t));
  rtn->type = KV_I64;
  rtn->data.i64 = i;
  return rtn;
}

kv_element_t *
new_kve_from_dbl(double i) {
  kv_element_t * rtn = malloc(sizeof(kv_element_t));
  rtn->type = KV_DBL;
  rtn->data.dbl = i;
  return rtn;
}

kv_element_t *
new_kve_from_ptr(void * i) {
  kv_element_t * rtn = malloc(sizeof(kv_element_t));
  rtn->type = KV_PTR;
  rtn->data.ptr = i;
  return rtn;
}

kv_element_t *
new_kve_from_str(string_t * i) {
  kv_element_t * rtn = malloc(sizeof(kv_element_t));
  rtn->type = KV_STR;
  rtn->data.str = *i;
  return rtn;
}

kv_element_t *
new_kve_from_str_static(char * s) {
  kv_element_t * rtn = malloc(sizeof(kv_element_t));
  rtn->type = KV_STR_STATIC;
  rtn->data.str.str = s;
  rtn->data.str.len = strlen(s);
  return rtn;
}

kv_element_t
kve_from_i64(int64_t i) {
  return (kv_element_t) {.type = KV_I64, .data.i64 = i};
}

kv_element_t
kve_from_dbl(double i) {
  return (kv_element_t) {.type = KV_DBL, .data.dbl = i};
}

kv_element_t
kve_from_ptr(void * i) {
  return (kv_element_t) {.type = KV_PTR, .data.ptr = i};
}

kv_element_t
kve_from_str(string_t * i) {
  return (kv_element_t) {.type = KV_STR, .data.str = *i};
}

kv_element_t
kve_from_str_static(char * i) {
  return (kv_element_t) {.type = KV_STR_STATIC, .data.str.str = i, .data.str.len = strlen(i), .data.str.max = strlen(i)};
}

int64_t
kve_get_i64(kv_element_t * e) {
  return e->data.i64;
}

double
kve_get_dbl(kv_element_t * e) {
  return e->data.dbl;
}

void *
kve_get_ptr(kv_element_t * e) {
  return e->data.ptr;
}

string_t *
kve_get_str(kv_element_t * e) {
  return &e->data.str;
}

char *
kve_get_cstr(kv_element_t * e) {
  return e->data.str.str;
}

kv_return_t 
kv_tracker_new(kv_element_t ** track) {
  IFNULL(track, return KV_RECEIVED_NULL);
  *track= calloc(1,sizeof(kv_element_t));
  IFNULL(*track, return KV_ALLOCATION_FAILED);
  return kv_tracker_init(*track);
}

kv_return_t 
kv_tracker_init(kv_element_t * track) {
  IFNULL(track, return KV_RECEIVED_NULL);

  track->type = KV_TRACKER;
  return kv_vector_new(&track->data.tracker.elements);
}

kv_return_t 
kv_tracker_free(kv_element_t ** track) {
  IFNULL(track, return KV_RECEIVED_NULL);
  IFNULL(*track, return KV_RECEIVED_NULL);

  kv_return_t rtn = kv_tracker_free_internal(*track);
  SAFETY(if(KV_SUCCESS != rtn) return rtn; );

  free(*track);
  SAFETY((*track) = NULL;);
}

kv_return_t 
kv_tracker_free_internal(kv_element_t * track) {
  int64_t stop = 0;
  kv_vector_length(track->data.tracker.elements, &stop);
  for(int64_t i = 0; i < stop; i++) {
    kv_element_t * cur;
    kv_vector_get(track->data.tracker.elements, i, &cur);
    if(cur) {
      kv_element_free(cur);
      free(cur);
    }
  }
  return kv_vector_free(&track->data.tracker.elements);
}

kv_return_t 
kv_tracker_hash(kv_element_t * track, int64_t * hash) {
  return kv_vector_hash(track->data.tracker.elements, hash);
}

kv_return_t 
kv_tracker_is(kv_element_t * a, kv_element_t * b) {
  if(a == b)
    return KV_EQUAL;
  else
    return KV_NOT_EQUAL;
}

kv_return_t 
kv_tracker_append(kv_element_t * track, kv_element_t * val) {
  return kv_vector_append(track->data.tracker.elements, val);
}

kv_element_t *
kv_tracker_none(kv_element_t * track) {
  kv_element_t * rtn = new_kve_from_i64(0);
  rtn->type = KV_NONE;
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_i64(kv_element_t * track, int64_t i) {
  kv_element_t * rtn = new_kve_from_i64(i);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_dbl(kv_element_t * track, double i) {
  kv_element_t * rtn = new_kve_from_dbl(i);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_ptr(kv_element_t * track, void * i) {
  kv_element_t * rtn = new_kve_from_ptr(i);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_str(kv_element_t * track, string_t * i) {
  kv_element_t * rtn = new_kve_from_str(i);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_str_static(kv_element_t * track, char * i) {
  kv_element_t * rtn = new_kve_from_str_static(i);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_vector(kv_element_t * track) {
  kv_element_t * rtn;
  kv_vector_new(&rtn);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_list(kv_element_t * track) {
  kv_element_t * rtn;
  kv_list_new(&rtn);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_element_t *
kv_tracker_store(kv_element_t * track) {
  kv_element_t * rtn;
  kv_store_new(&rtn);
  kv_tracker_append(track, rtn);
  return rtn;
}

kv_return_t
kv_element_free(kv_element_t * element) {
  IFNULL((element), return KV_RECEIVED_NULL);

  kv_return_t rtn = KV_SUCCESS;
  switch((element)->type) {
    case KV_NONE:
    case KV_I64:
    case KV_DBL:
    case KV_PTR:
    case KV_STR_STATIC:
      /* NO INTERNAL FREE FOR THESE TYPES */
    break;

    case KV_STR: {
      string_free_internal(&(element)->data.str);
    } break;

    case KV_KV: {
      rtn = kv_store_free_internal(element);
    } break;

    case KV_LIST: {
      rtn = kv_list_free_internal(element);
    } break;

    case KV_VECTOR: {
      rtn = kv_vector_free_internal(element);
    } break;

    case KV_TRACKER: {
      rtn = kv_tracker_free_internal(element);
    } break;
  }
  element->type = KV_NONE;
  SAFETY(if(KV_SUCCESS != rtn) return rtn; );
  return KV_SUCCESS;
}

kv_return_t
kv_element_equal(kv_element_t * a, kv_element_t * b) {
  IFNULL(a, return KV_RECEIVED_NULL);
  IFNULL(b, return KV_RECEIVED_NULL);

  if(a->type != b->type) {
    if((a->type != KV_STR_STATIC && a->type != KV_STR) || (b->type != KV_STR && b->type != KV_STR_STATIC))
    return KV_NOT_EQUAL;
  }

  switch(a->type) {
    case KV_NONE: 
    case KV_I64: {
      if(a->data.i64 == b->data.i64) return KV_EQUAL; else return KV_NOT_EQUAL;
    } break;

    case KV_DBL: {
      if(a->data.dbl == b->data.dbl) return KV_EQUAL; else return KV_NOT_EQUAL;
    } break;

    case KV_PTR: {
      if(a->data.ptr == b->data.ptr) return KV_EQUAL; else return KV_NOT_EQUAL;
    } break;

    case KV_STR_STATIC:
    case KV_STR: {
      if(string_equal(&a->data.str, &b->data.str)) return KV_EQUAL; else return KV_NOT_EQUAL;
    } break;

    case KV_KV: {
      return kv_store_equal(a,b);
    } break;

    case KV_LIST: {
      return kv_list_equal(a,b);
    } break;

    case KV_VECTOR: {
      return kv_vector_equal(a,b);
    } break;

    case KV_TRACKER: {
      return kv_tracker_is(a,b);
    } break;

    default: {
      FATAL("UNKNOWN TYPE %d", a->type);
    }
  }
}

kv_return_t
kv_element_hash(kv_element_t * a, uint64_t * hash) {
  IFNULL(*a, return KV_RECEIVED_NULL);

  uint64_t out = 0;

  switch(a->type) {
    case KV_NONE: 
    case KV_DBL:
    case KV_PTR:
    case KV_I64: {
      *hash = (*hash) ^ mix(a->data.i64);
    } break;

    case KV_STR_STATIC: 
    case KV_STR: {
      *hash = (*hash) ^ string_hash(&a->data.str);
    } break;

    case KV_KV: {
      kv_return_t rtn = kv_store_hash(a, hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn);
    } break;

    case KV_LIST: {
      kv_return_t rtn = kv_list_hash(a, hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn);
    } break;

    case KV_VECTOR: {
      kv_return_t rtn = kv_vector_hash(a, hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn);
    } break;

    case KV_TRACKER: {
      kv_return_t rtn = kv_tracker_hash(a, hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn);
    } break;

    default: {
      FATAL("UNKNOWN TYPE %d", a->type);
    }
  }
  return KV_SUCCESS;
}

kv_return_t
kv_element_print(kv_element_t * a, int64_t indent) {
  IFNULL(*a, return KV_RECEIVED_NULL);

  switch(a->type) {
    case KV_NONE: {
      print_indent(indent); printf("NONE\n");
    } break;

    case KV_I64: {
      print_indent(indent); printf("I64: %ld\n", (long int)a->data.i64);
    } break;

    case KV_DBL: {
      print_indent(indent); printf("DBL: %lf\n", a->data.dbl);
    } break;

    case KV_PTR: {
      print_indent(indent); printf("PTR: %p\n", a->data.ptr);
    } break;

    case KV_STR_STATIC: 
    case KV_STR: {
      print_indent(indent); printf("STR: %.*s\n", a->data.str.len, a->data.str.str);
    } break;

    case KV_KV: {
      kv_store_print(a, indent);
    } break;

    case KV_LIST: {
      kv_list_print(a, indent);
    } break;

    case KV_VECTOR: {
  /* TODO */
    } break;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_LIST FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_list_new(kv_element_t ** rtn) {
  IFNULL(rtn, return KV_RECEIVED_NULL);
  *rtn = calloc(1,sizeof(kv_element_t));
  IFNULL(*rtn, return KV_ALLOCATION_FAILED);
  return kv_list_init(*rtn);
}

kv_return_t
kv_list_init(kv_element_t * list) {
  IFNULL(list, return KV_RECEIVED_NULL);

  list->type = KV_LIST;
  list->data.list.head = NULL;
  list->data.list.tail = NULL;
  list->data.list.length = 0;
  return KV_SUCCESS;
}

kv_return_t
kv_list_free_internal(kv_element_t * list) {
  IFNULL(list, return KV_RECEIVED_NULL);

  kv_list_element_t * tmp, * cur = list->data.list.head;
  while(cur) {
    tmp = cur;
    cur = cur->next;
    free(tmp);
  }

  SAFETY(list->data.list.head = NULL);
  SAFETY(list->data.list.tail = NULL);
  SAFETY(list->data.list.length = 0);
  return KV_SUCCESS;
}

kv_return_t
kv_list_free(kv_element_t ** list) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(*list, return KV_RECEIVED_NULL);

  kv_return_t rtn = kv_list_free_internal(*list);
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  free(*list);
  SAFETY(*list = NULL);

  return KV_SUCCESS;
}

kv_return_t
kv_list_equal(kv_element_t * a, kv_element_t * b) {
  IFNULL(a, return KV_RECEIVED_NULL);
  IFNULL(b, return KV_RECEIVED_NULL);

  kv_return_t rtn = KV_EQUAL;
  kv_list_element_t * ac = a->data.list.head, * bc = b->data.list.head;

  while(NULL != ac && NULL != bc) {
    rtn = kv_element_equal(ac->data, bc->data);

    if(KV_EQUAL != rtn)
      return rtn;
      
    ac = ac->next;
    bc = bc->next;
  }

  return KV_EQUAL;
}

kv_return_t
kv_list_print(kv_element_t * list, int64_t indent) {
  IFNULL(list, return KV_RECEIVED_NULL);

  print_indent(indent++); printf("LIST\n");
  kv_list_element_t * cur = list->data.list.head;

  while(NULL != cur) {
    kv_element_print(cur->data, indent);
    cur = cur->next;
  }

  return KV_SUCCESS;
}

kv_return_t
kv_list_hash(kv_element_t * list, uint64_t * hash) {
  IFNULL(list, return KV_RECEIVED_NULL);

  kv_list_element_t * cur = list->data.list.head;
  while(cur) {
    kv_return_t rtn = kv_element_hash(cur->data, hash);
    SAFETY(if(KV_SUCCESS != rtn) return rtn;);
    cur = cur->next;
  }
  
  return KV_SUCCESS;
}

kv_return_t
kv_list_length(kv_element_t * list, int64_t * length) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(length, return KV_RECEIVED_NULL);

  *length = list->data.list.length;
  return KV_SUCCESS;
}

kv_return_t
kv_list_find(kv_element_t * list, kv_element_t * val, kv_list_element_t ** found) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);
  IFNULL(found, return KV_RECEIVED_NULL);

  kv_list_element_t * cur = list->data.list.head;
  while(cur) {
    if(KV_EQUAL == kv_element_equal(cur->data, val)) {
      *found = cur;
      return KV_SUCCESS;
    }
    cur = cur->next;
  }
  return KV_DOES_NOT_EXIST;
}

kv_return_t
kv_list_insert_before(kv_element_t * list, kv_element_t * val, kv_list_element_t * found) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);
  IFNULL(found, return KV_RECEIVED_NULL);

  kv_list_element_t * in = malloc(sizeof(kv_list_element_t));
  IFNULL(in, return KV_ALLOCATION_FAILED);

  in->data = val;
  in->next = found;
  in->prev = found->prev;
  found->prev = in;
  if(in->prev)
    in->prev->next = in;
  else
    list->data.list.head = in;

  list->data.list.length++;

  return KV_SUCCESS;
}

kv_return_t
kv_list_insert_after(kv_element_t * list, kv_element_t * val, kv_list_element_t * found) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);
  IFNULL(found, return KV_RECEIVED_NULL);

  kv_list_element_t * in = malloc(sizeof(kv_list_element_t));
  IFNULL(in, return KV_ALLOCATION_FAILED);

  in->data = val;
  in->next = found->next;
  in->prev = found;
  found->next = in;
  if(in->next)
    in->next->prev = in;
  else
    list->data.list.tail = in;

  list->data.list.length++;

  return KV_SUCCESS;
}

kv_return_t
kv_list_insert_first(kv_element_t * list, kv_element_t * val) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  kv_list_element_t * in = malloc(sizeof(kv_list_element_t));
  IFNULL(in, return KV_ALLOCATION_FAILED);

  in->data = val;
  in->next = list->data.list.head;
  in->prev = NULL;
  list->data.list.head = in;
  if(in->next)
    in->next->prev = in;
  else
    list->data.list.tail = in;

  list->data.list.length++;

  return KV_SUCCESS;
}

kv_return_t
kv_list_insert_last(kv_element_t * list, kv_element_t * val) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  kv_list_element_t * in = malloc(sizeof(kv_list_element_t));
  IFNULL(in, return KV_ALLOCATION_FAILED);

  in->data = val;
  in->prev = list->data.list.tail;
  in->next = NULL;
  list->data.list.tail = in;
  if(in->prev)
    in->prev->next = in;
  else
    list->data.list.head = in;

  list->data.list.length++;

  return KV_SUCCESS;
}

kv_return_t
kv_list_pop_first(kv_element_t * list, kv_element_t ** out) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(out, return KV_RECEIVED_NULL);

  if(list->data.list.length <= 0) {
    SAFETY(*out = NULL);
    return KV_DOES_NOT_EXIST;
  }

  kv_list_element_t * rem = list->data.list.head;
  *out = rem->data;
  if((rem)->next) {
    (rem)->next->prev = NULL;
  } else {
    list->data.list.tail = NULL;
  }
  list->data.list.head = (rem)->next;

  free(rem);

  list->data.list.length--;
  return KV_SUCCESS;
}

kv_return_t
kv_list_pop_last(kv_element_t * list, kv_element_t ** out) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(out, return KV_RECEIVED_NULL);

  if(list->data.list.length <= 0) {
    SAFETY(*out = NULL);
    return KV_DOES_NOT_EXIST;
  }

  kv_list_element_t * rem = list->data.list.tail;
  *out = rem->data;
  if((rem)->prev) {
    (rem)->prev->next = NULL;
  } else {
    list->data.list.head = NULL;
  }
  list->data.list.tail = (rem)->prev;

  free(rem);

  list->data.list.length--;
  return KV_SUCCESS;
}

kv_return_t
kv_list_remove(kv_element_t * list, kv_list_element_t * found) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(found, return KV_RECEIVED_NULL);

  if(found->prev)
    found->prev->next = found->next;
  else
    list->data.list.head = found->next;

  if(found->next)
    found->next->prev = found->prev;
  else
    list->data.list.tail = found->prev;

  free(found);

  list->data.list.length--;

  return KV_SUCCESS;
}

kv_return_t
kv_list_first(kv_element_t * list, kv_list_element_t ** iterator, kv_element_t ** data) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(iterator, return KV_RECEIVED_NULL);
  IFNULL(data, return KV_RECEIVED_NULL);

  *iterator = list->data.list.head;
  if(*iterator)
    *data = (*iterator)->data;
  else
    *data = NULL;

  return KV_SUCCESS;
}

kv_return_t
kv_list_next(kv_element_t * list, kv_list_element_t ** iterator, kv_element_t ** data) {
  IFNULL(list, return KV_RECEIVED_NULL);
  IFNULL(iterator, return KV_RECEIVED_NULL);
  IFNULL(data, return KV_RECEIVED_NULL);

  *iterator = (*iterator)->next;
  if(*iterator)
    *data = (*iterator)->data;
  else
    *data = NULL;

  return KV_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_VECTOR FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_vector_new_with_size(kv_element_t ** rtn, int64_t size) {
  IFNULL(rtn, return KV_RECEIVED_NULL);
  *rtn = calloc(1, sizeof(kv_element_t));
  IFNULL(*rtn, return KV_ALLOCATION_FAILED);

  return kv_vector_init_with_size(*rtn, size);
}

kv_return_t
kv_vector_new(kv_element_t ** rtn) {
  return kv_vector_new_with_size(rtn, KV_VECTOR_DEFAULT_SIZE);
}

kv_return_t
kv_vector_init_with_size(kv_element_t * rtn, int64_t size) {
  IFNULL(rtn, return KV_RECEIVED_NULL);
  rtn->data.vec.arr = calloc(size, sizeof(kv_element_t *));
  IFNULL(rtn->data.vec.arr, return KV_ALLOCATION_FAILED);

  rtn->type = KV_VECTOR;
  rtn->data.vec.max= size;
  rtn->data.vec.length = 0;
  return KV_SUCCESS;
}

kv_return_t
kv_vector_init(kv_element_t * rtn) {
  return kv_vector_init_with_size(rtn, KV_VECTOR_DEFAULT_SIZE);
}

kv_return_t
kv_vector_free_internal(kv_element_t * vec) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(vec->data.vec.arr, return KV_RECEIVED_NULL);

  free(vec->data.vec.arr);
  SAFETY(vec->data.vec.arr = NULL;);

  return KV_SUCCESS;
}

kv_return_t
kv_vector_free(kv_element_t ** vec) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(*vec, return KV_RECEIVED_NULL);
  kv_vector_free_internal(*vec);
  free(*vec);
  SAFETY(*vec = NULL;);
  return KV_SUCCESS;
}

kv_return_t
kv_vector_equal(kv_element_t * a, kv_element_t * b) {
  IFNULL(a, return KV_RECEIVED_NULL);
  IFNULL(b, return KV_RECEIVED_NULL);

  if(a->data.vec.length != b->data.vec.length) {
    return KV_NOT_EQUAL;
  }

  for(uint64_t i = 0; i < a->data.vec.length; i++) {
    kv_return_t rtn = kv_element_equal(a->data.vec.arr[i], b->data.vec.arr[i]);
    if(KV_EQUAL != rtn || (a->data.vec.arr[i] == KV_NONE_PTR && b->data.vec.arr[i] == KV_NONE_PTR))
      return KV_NOT_EQUAL;
  }
  return KV_EQUAL;
}

kv_return_t
kv_vector_hash(kv_element_t * vec, uint64_t * hash) {
  IFNULL(vec, return KV_RECEIVED_NULL);

  for(uint64_t i = 0; i < vec->data.vec.length; i++) {
    kv_return_t rtn = kv_element_hash(vec->data.vec.arr[i], hash);
    SAFETY(if(KV_SUCCESS != rtn) return rtn;);
  }
  
  return KV_SUCCESS;
}

kv_return_t
kv_vector_get(kv_element_t * vec, int64_t index, kv_element_t ** out) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(vec->data.vec.arr, return KV_RECEIVED_NULL);
  IFNULL(out, return KV_RECEIVED_NULL);

  SAFETY(if(index >= vec->data.vec.length) return KV_OUT_OF_BOUNDS;);

  *out = vec->data.vec.arr[index];

  return KV_SUCCESS;
}

kv_return_t
kv_vector_set(kv_element_t * vec, int64_t index, kv_element_t * val) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(vec->data.vec.arr, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  SAFETY(if(index >= vec->data.vec.length) return KV_OUT_OF_BOUNDS;);

  kv_return_t rtn;
  if(index + 1 >= vec->data.vec.length) {
    kv_vector_resize(vec, index+1);
  }

  vec->data.vec.arr[index] = val;
  return KV_SUCCESS;
}

kv_return_t
kv_vector_append(kv_element_t * vec, kv_element_t * val) {
  /* checks handled by set */
  return kv_vector_set(vec, vec->data.vec.length, val);
}

kv_return_t
kv_vector_resize(kv_element_t * vec, int64_t new_size) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(vec->data.vec.arr, return KV_RECEIVED_NULL);

  if(new_size > vec->data.vec.max) {
    int64_t new_max = vec->data.vec.max;
    while(new_max < new_size) {
      new_max *= KV_VECTOR_GROW_RATE;
    }

    kv_element_t ** new_arr = calloc(new_max, sizeof(kv_element_t *));
    IFNULL(new_arr, return KV_ALLOCATION_FAILED);

    for(int64_t i = 0; i < vec->data.vec.length; i++) {
      new_arr[i] = vec->data.vec.arr[i];
    }

    free(vec->data.vec.arr);
    vec->data.vec.arr = new_arr;
    vec->data.vec.max = new_max;
  } 
  vec->data.vec.length = new_size;

  return KV_SUCCESS;
}

kv_return_t
kv_vector_length(kv_element_t * vec, int64_t * length) {
  IFNULL(vec, return KV_RECEIVED_NULL);
  IFNULL(length, return KV_RECEIVED_NULL);
  *length = vec->data.vec.length;
  return KV_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * KV_STORE FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

kv_return_t
kv_store_new(kv_element_t ** kv) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  *kv = calloc(1, sizeof(kv_element_t));
  IFNULL(*kv, return KV_ALLOCATION_FAILED);
  return kv_store_init(*kv);
}

kv_return_t
kv_store_init(kv_element_t * kv) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  kv->type = KV_KV;
  kv->data.kv.keys = calloc(2 * KV_STORE_DEFAULT_SIZE, sizeof(kv_element_t *));
  IFNULL(kv->data.kv.keys, return KV_ALLOCATION_FAILED);
  kv->data.kv.vals = kv->data.kv.keys + KV_STORE_DEFAULT_SIZE;
  kv->data.kv.size = KV_STORE_DEFAULT_SIZE;
  kv->data.kv.mask = KV_STORE_DEFAULT_SIZE - 1;
  kv->data.kv.elements = 0;
  kv->data.kv.removed = 0;
  return KV_SUCCESS;
}

kv_return_t
kv_store_free_internal(kv_element_t * kv) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(kv->data.kv.keys, return KV_RECEIVED_NULL);
  free(kv->data.kv.keys);
  SAFETY(kv->data.kv.keys = NULL);
  SAFETY(kv->data.kv.vals = NULL);
  SAFETY(kv->data.kv.elements = 0);
  SAFETY(kv->data.kv.removed = 0);
  SAFETY(kv->data.kv.size = 0);
  SAFETY(kv->data.kv.mask = 0);
  return KV_SUCCESS;
}

kv_return_t
kv_store_free(kv_element_t ** kv) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  kv_return_t rtn = kv_store_free_internal(*kv);
  SAFETY(if(KV_SUCCESS != rtn) return rtn; );
  free(*kv);
  SAFETY(*kv = NULL; );
  return KV_SUCCESS;
}

kv_return_t
kv_store_equal(kv_element_t * a, kv_element_t * b) {
  IFNULL(a, return KV_RECEIVED_NULL);
  IFNULL(b, return KV_RECEIVED_NULL);

  if(a->data.kv.elements != b->data.kv.elements)
    return KV_NOT_EQUAL;

  for(int64_t i = 0; i < a->data.kv.size; i++) {
    if(a->data.kv.keys[i] != KV_DELETED_PTR && a->data.kv.keys[i] != KV_NONE_PTR) {
      kv_element_t * bv;
      if(KV_DOES_NOT_EXIST == kv_store_get(b, a->data.kv.keys[i], &bv)) {
	return KV_NOT_EQUAL;
      }
      if(KV_NOT_EQUAL == kv_element_equal(a->data.kv.vals[i], bv)) {
	return KV_NOT_EQUAL;
      }
    }
  }

  return KV_EQUAL;
}

kv_return_t
kv_store_print(kv_element_t * kv, int indent) {
  IFNULL(kv, return KV_RECEIVED_NULL);

  print_indent(indent++); printf("KV_STORE\n");
  for(int64_t i = 0; i < kv->data.kv.size; i++) {
    if(kv->data.kv.keys[i] != KV_DELETED_PTR && kv->data.kv.keys[i] != KV_NONE_PTR) {
      print_indent(indent); printf("KEY\n");
      kv_element_print(kv->data.kv.keys[i], indent+1);
      print_indent(indent); printf("VAL\n");
      kv_element_print(kv->data.kv.vals[i], indent+1);
    }
  }

  return KV_SUCCESS;
}

kv_return_t
kv_store_hash(kv_element_t * kv, uint64_t * hash) {
  IFNULL(kv, return KV_RECEIVED_NULL);

  for(uint64_t i = 0; i < kv->data.kv.size; i++) {
    if(kv->data.kv.keys[i] != KV_DELETED_PTR && kv->data.kv.keys[i] != KV_NONE_PTR) {
      kv_return_t rtn = kv_element_hash(kv->data.kv.keys[i], hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn;);
      rtn = kv_element_hash(kv->data.kv.vals[i], hash);
      SAFETY(if(KV_SUCCESS != rtn) return rtn;);
    }
  }
  
  return KV_SUCCESS;
}

kv_return_t
kv_store_get(kv_element_t * kv, kv_element_t * key, kv_element_t ** val) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  int64_t index = 0;
  kv_return_t rtn = kv_element_hash(key, &index); 
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  index = index & kv->data.kv.mask;
  while(kv->data.kv.keys[index] != KV_NONE_PTR && 
    (kv->data.kv.keys[index] == KV_DELETED_PTR ||
    kv_element_equal(key, kv->data.kv.keys[index]) != KV_EQUAL)) {
    index = (index+1) & kv->data.kv.mask;
  }

  if(kv->data.kv.keys[index] != KV_NONE_PTR) {
    *val = kv->data.kv.vals[index];
    return KV_SUCCESS;
  } else {
    SAFETY(*val = NULL);
    return KV_DOES_NOT_EXIST;
  }
}

kv_return_t
kv_store_set(kv_element_t * kv, kv_element_t * key, kv_element_t * val) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  if((kv->data.kv.elements + kv->data.kv.removed) > (KV_STORE_THRESHOLD * kv->data.kv.size)) {
    kv_return_t rtn = kv_store_expand(kv);
    SAFETY(if(KV_SUCCESS != rtn) return rtn);
  }

  int64_t index = 0;
  kv_return_t rtn = kv_element_hash(key, &index); 
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  index = index & kv->data.kv.mask;
  int64_t deleted_index = -1;
  while(kv->data.kv.keys[index] != KV_NONE_PTR) {
    if(kv->data.kv.keys[index] == KV_DELETED_PTR) {
      deleted_index = index;
    } else if(kv_element_equal(key, kv->data.kv.keys[index]) == KV_EQUAL) {
      break;
    }
    index = (index+1) & kv->data.kv.mask;
  }

  if(kv->data.kv.keys[index] == KV_NONE_PTR) {
    if(deleted_index != -1) {
      kv->data.kv.keys[deleted_index] = key;
      kv->data.kv.vals[deleted_index] = val;
    } else {
      kv->data.kv.keys[index] = key;
      kv->data.kv.vals[index] = val;
    }
    kv->data.kv.elements++;
    return KV_SUCCESS;
  } else {
    kv->data.kv.vals[index] = val;
    return KV_SUCCESS;
  }
  return KV_SUCCESS;
}

kv_return_t
kv_store_remove(kv_element_t * kv, kv_element_t * key) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);

  if((kv->data.kv.elements + kv->data.kv.removed) > (KV_STORE_THRESHOLD * kv->data.kv.size)) {
    kv_return_t rtn = kv_store_expand(kv);
    SAFETY(if(KV_SUCCESS != rtn) return rtn);
  }

  int64_t index = 0;
  kv_return_t rtn = kv_element_hash(key, &index); 
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  index = index & kv->data.kv.mask;
  int64_t deleted_index = -1;
  while(kv->data.kv.keys[index] != KV_NONE_PTR && 
    (kv->data.kv.keys[index] == KV_DELETED_PTR || 
      kv_element_equal(key, kv->data.kv.keys[index]) != KV_EQUAL)) {
     
    index = (index+1) & kv->data.kv.mask;
  }

  if(kv->data.kv.keys[index] != KV_NONE_PTR && kv->data.kv.keys[index] != KV_DELETED_PTR) {
    kv->data.kv.elements--;
    kv->data.kv.removed++;
    kv->data.kv.keys[index] = KV_DELETED_PTR;
    kv->data.kv.vals[index] = KV_DELETED_PTR;
    return KV_SUCCESS;
  } else {
    return KV_DOES_NOT_EXIST;
  }
}

kv_return_t
kv_store_contains(kv_element_t * kv, kv_element_t * key) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);

  int64_t index = 0;
  kv_return_t rtn = kv_element_hash(key, &index); 
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  index = index & kv->data.kv.mask;
  int64_t deleted_index = -1;
  while(kv->data.kv.keys[index] != KV_NONE_PTR && 
    (kv->data.kv.keys[index] == KV_DELETED_PTR || 
      kv_element_equal(key, kv->data.kv.keys[index]) != KV_EQUAL)) {
     
    index = (index+1) & kv->data.kv.mask;
  }

  if(kv->data.kv.keys[index] != KV_NONE_PTR && kv->data.kv.keys[index] != KV_DELETED_PTR) {
    return KV_SUCCESS;
  } else {
    return KV_DOES_NOT_EXIST;
  }
}

kv_return_t
kv_store_expand(kv_element_t * kv) {
  IFNULL(kv, return KV_RECEIVED_NULL);

  kv_element_t tmp;
  /* if this is really full of deleted elements, copy but don't expand*/
  tmp.data.kv.size = kv->data.kv.removed > kv->data.kv.elements ? kv->data.kv.size : kv->data.kv.size * KV_STORE_GROW_RATE;
  tmp.data.kv.mask = tmp.data.kv.size - 1;
  tmp.data.kv.removed = 0;
  tmp.data.kv.elements = 0;

  tmp.data.kv.keys = calloc(tmp.data.kv.size * 2, sizeof(kv_element_t *));
  IFNULL(tmp.data.kv.keys, return KV_ALLOCATION_FAILED);
  tmp.data.kv.vals = tmp.data.kv.keys + tmp.data.kv.size;

  for(int64_t i = 0; i < kv->data.kv.size; i++) {
    if(kv->data.kv.keys[i] != KV_DELETED_PTR && kv->data.kv.keys[i] != KV_NONE_PTR) {
      kv_store_set(&tmp, kv->data.kv.keys[i], kv->data.kv.vals[i]);
    }
  }

  kv_return_t rtn = kv_store_free_internal(kv);
  SAFETY(if(KV_SUCCESS != rtn) return rtn;);

  *kv = tmp;
  return KV_SUCCESS;
}

kv_return_t
kv_store_length(kv_element_t * kv, int64_t * out) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(out, return KV_RECEIVED_NULL);

  *out = kv->data.kv.elements;
  return KV_SUCCESS;
}

/* TODO
kv_return_t
kv_store_contains_value(kv_element_t * kv, kv_element_t * key) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);
  return KV_SUCCESS;
}
*/

kv_return_t
kv_store_first(kv_element_t * kv, kv_element_t *** ik, kv_element_t *** iv, kv_element_t ** key, kv_element_t ** val) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(ik, return KV_RECEIVED_NULL);
  IFNULL(iv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  (*ik) = kv->data.kv.keys;
  (*iv) = kv->data.kv.vals;

  while((((**ik) == KV_NONE_PTR) || ((**ik) == KV_DELETED_PTR)) && ((*ik) < kv->data.kv.keys + kv->data.kv.size)) {
    (*ik)++;
    (*iv)++;
  }

  if((*ik) == kv->data.kv.keys + kv->data.kv.size) {
    SAFETY(*ik = NULL);
    SAFETY(*iv = NULL);
    *key = NULL;
    *val = NULL;
    return KV_DOES_NOT_EXIST;
  } else {
    *key = **ik;
    *val = **iv;
    return KV_SUCCESS;
  }
}

kv_return_t
kv_store_next(kv_element_t * kv, kv_element_t *** ik, kv_element_t *** iv, kv_element_t ** key, kv_element_t ** val) {
  IFNULL(kv, return KV_RECEIVED_NULL);
  IFNULL(ik, return KV_RECEIVED_NULL);
  IFNULL(iv, return KV_RECEIVED_NULL);
  IFNULL(key, return KV_RECEIVED_NULL);
  IFNULL(val, return KV_RECEIVED_NULL);

  (*ik)++;
  (*iv)++;

  while((((**ik) == KV_NONE_PTR) || ((**ik) == KV_DELETED_PTR)) && ((*ik) < kv->data.kv.keys + kv->data.kv.size)) {
    (*ik)++;
    (*iv)++;
  }

  if((*ik) == kv->data.kv.keys + kv->data.kv.size) {
    SAFETY(*ik = NULL);
    SAFETY(*iv = NULL);
    *key = NULL;
    *val = NULL;
    return KV_DOES_NOT_EXIST;
  } else {
    *key = **ik;
    *val = **iv;
    return KV_SUCCESS;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ * 
 * OTHER FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#if defined(TYPE)
#undef TYPE
#endif
#define TYPE(X) #X
static const char * errors [] = {
  KV_RETURN_TYPES
};
#undef TYPE

const char *
kv_error_string(kv_return_t rtn) {
  const char * unknwon = "UNKNOWN ERROR NUMBER";
  if(rtn < KV_RETURN_MAX && rtn >= 0)
    return errors[rtn];
  else
    return unknwon;
}

kv_return_t
kv_from_args(int argc, char ** argv, kv_element_t * tracker, kv_element_t ** top_level) {
  IFNULL(argv, return KV_RECEIVED_NULL;);
  *top_level = NULL;

  kv_element_t * sections; kv_store_new(&sections);
  kv_element_t * cur_section = sections;
  kv_tracker_append(tracker, sections);
  kv_element_t * cur_none = kv_tracker_none(tracker);

  string_t * cur_val;
  string_t * cur_key;
  string_t * cur_store_key;
  string_t * cur_store_val;
  kv_element_t * cur_list;
  kv_element_t * cur_store;
  int last_was_space = 0;

  for(uint64_t i = 0; i < argc; i++) {
    char * s = argv[i];
    char c = s[0];
    while(1) {
      switch(s[0]) {
	/* eat leading spaces */
	case ' ': {
	} break;

	case ',':
	case ':': 
	case ']':
	case '=': {
	  ERROR("Arguments: unexpected %c", c);
	  return KV_PARSE_ERROR;
	} break;

	/* section */
	case '[': 
	case '@': {
	  cur_store = sections;
	  kv_store_new(&cur_section);
	  cur_val = string_new();
	  c = *(++s);
	  while(1) {
	    switch(c) {
	      case -1:
	      case ';':
	      case '#': 
	      case '\r':
	      case '\n': 
	      case '=': 
	      case '[': {
		ERROR("Arguments: unexpected %c", c);
		string_free(cur_val);
		return KV_PARSE_ERROR;
	      } break;

	      case '\0':
	      case ']': {
		if(cur_val->len > 0) {
		  string_prepend_char(cur_val, '.');
		  kv_element_t *rtn, str = kve_from_str(cur_val);
		  if(KV_DOES_NOT_EXIST != kv_store_get(cur_store, &str, &rtn)) {
		    kv_store_free(&cur_section);
		    cur_section = rtn;
		  } else {
		    kv_tracker_append(tracker, cur_section);
		    kv_store_set(cur_store, kv_tracker_str(tracker,cur_val), cur_section);
		  }
		} else {
		  kv_store_free(&cur_section);
		  string_free(cur_val);
		}
		goto argdone;
	      } break;

	      case '\t':
	      case ' ':
	      case '.': 
	      case '|': 
	      case ':': {
		if(cur_val->len > 0) {
		  string_prepend_char(cur_val, '.');
		  kv_element_t *rtn, str = kve_from_str(cur_val);
		  if(KV_DOES_NOT_EXIST != kv_store_get(cur_store, &str, &rtn)) {
		    kv_store_free(&cur_section);
		    cur_section = rtn;
		  } else {
		    kv_tracker_append(tracker, cur_section);
		    kv_store_set(cur_store, kv_tracker_str(tracker,cur_val), cur_section);
		  }
		  cur_store = cur_section;
		  kv_store_new(&cur_section);
		  cur_val = string_new();
		}
	      } break;

	      default: {
		string_append_char(cur_val, c);
	      }
	    }
	    c = *(++s);
	  }

	} break;

	default: {
	  goto argeatkey;
	} break;
      }
      c = *(++s);
    }

  argeatkey:
    cur_key = string_new();
    while(1) {
      switch(c) {
	case -1:
	case ';':
	case '#': 
	case '\r':
	case '\0':
	case '\n': {
	  kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), cur_none);
	  goto argdone;
	} break;

	case ',':
	case ':': 
	case ']':
	case '[': {
	  string_free(cur_key);
	  ERROR("Argument: unexpected %c", c);
	  return KV_PARSE_ERROR;
	} break;

	case '=': { 
	  c = *(++s);
	  goto argeatvalue;
	}

	case ' ':
	case '\t': {
	  /* eat whitespace */
	} break;

	default: {
	  string_append_char(cur_key, c);
	}
      }
      c = *(++s);
    }

  argeatvalue:
    cur_val = string_new();
    last_was_space = 0;
    while(1) {
      switch(c) {
	case -1:
	case ';':
	case '#': 
	case '\r':
	case '\0': 
	case '\n': {
	  kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), kv_tracker_str(tracker,cur_val));
	  goto argdone;
	} break;

	case ']':
	case '=': 
	case '[': {
	  ERROR("Argument: unexpected %c", c);
	  return KV_PARSE_ERROR;
	} break;

	case ',': {
	  goto argeatlist;
	} break;

	case '|':
	case ':': {
	  goto argeatkeyvalue;
	} break;

	case ' ':
	case '\t': {
	  /* space separated list? */
	  if(cur_val->len > 0) {
	    last_was_space = 1;
	  } else {
	    /* eat whitespace */
	  }
	} break;

	default: {
	  if(!last_was_space)
	    string_append_char(cur_val, c);
	  else
	    goto argeatlist;
	}
      }
      c = *(++s);
    }

  argeatlist:
    kv_list_new(&cur_list);
    kv_tracker_append(tracker, cur_list);
    kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
    cur_val = string_new();
    while(1) {
      switch(c) {
	case -1:
	case ';':
	case '#': 
	case '\0': 
	case '\r':
	case '\n': {
	  if(cur_val->len > 0)
	    kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
	  else
	    string_free(cur_val);
	  kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), cur_list);
	  goto argdone;
	} break;

	case ']':
	case '=': 
	case '[': {
	  ERROR("Argument: unexpected %c", c);
	  string_free(cur_val);
	  return KV_PARSE_ERROR;
	} break;

	case ' ':
	case '\t': 
	case ',': {
	  if(cur_val->len > 0) {
	    kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
	    cur_val = string_new();
	  }
	} break;

	case ':': {
	  ERROR("Argument: unexpected %c", c);
	  return KV_PARSE_ERROR;
	} break;

	default: {
	  string_append_char(cur_val, c);
	}
      }
      c = *(++s);
    }

  argeatkeyvalue:
    kv_store_new(&cur_store);
    kv_tracker_append(tracker, cur_store);
    cur_store_key = cur_val;
    cur_store_val = NULL;
    cur_val = string_new();
    last_was_space = 0;
    while(1) {
      switch(c) {
	case -1:
	case ';':
	case '#': 
	case '\0': 
	case '\r':
	case '\n': {
	  if(cur_store_key->len > 0 && cur_store_val) {
	    kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	  } else {
	    string_free(cur_store_key);
	    if(cur_store_val)
	      string_free(cur_store_val);
	  }
	  kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), cur_store);
	  goto argdone;
	} break;

	case ']':
	case '=': 
	case '[': {
	  ERROR("Arguement: unexpected %c", c);
	  return KV_PARSE_ERROR;
	} break;

	case ' ':
	case '\t':  {
	  last_was_space = 1;
	} break;

	case ',': {
	  if(cur_store_key->len > 0 && cur_store_val) {
	    kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	  } else {
	    string_free(cur_store_key);
	    if(cur_store_val)
	      string_free(cur_store_val);
	  }
	  cur_val = string_new();
	  cur_store_key = cur_val;
	  cur_store_val = NULL;
	} break;

	case '|': 
	case ':': {
	  if(!cur_store_val) {
	    cur_val = string_new();
	    cur_store_val = cur_val;
	  } else {
	    ERROR("Argument: unexpected %c key is %.*s val is %.*s", c, cur_store_key->len, cur_store_key->str, 
	      cur_store_val ? cur_store_val->len : 0, cur_store_val ? cur_store_val->str : NULL);
	    return KV_PARSE_ERROR;
	  }
	} break;

	default: {
	  if(last_was_space) {
	    if(cur_store_key->len > 0 && cur_store_val && cur_store_val->len) {
	      kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	      cur_val = string_new();
	      cur_store_key = cur_val;
	      cur_store_val = NULL;
	    } 
	  }
	  string_append_char(cur_val, c);
	  last_was_space = 0;
	}
      }
      c = *(++s);
    }
  argdone:
    c = '\0';
  }
  *top_level = sections;
  return KV_SUCCESS;
}

kv_return_t
kv_from_ini(FILE * ini, kv_element_t * tracker, kv_element_t ** top_level) {
  IFNULL(top_level, return KV_RECEIVED_NULL;);
  IFNULL(tracker, return KV_RECEIVED_NULL;);

  *top_level = NULL;

  kv_element_t * sections; kv_store_new(&sections);
  kv_element_t * cur_section; kv_store_new(&cur_section);
  kv_tracker_append(tracker, sections);
  kv_tracker_append(tracker, cur_section);

  string_t * cur_val;
  string_t * cur_key;
  string_t * cur_store_key;
  string_t * cur_store_val;
  kv_element_t * cur_list;
  kv_element_t * cur_store;
  int c;
  int line_num = 0;
  int last_was_space = 0;

  if(c == ';' || c == '#') {
eatcomments:
    while(!feof(ini) && c != '\n')
      c = getc(ini);
  }
  
eatwhitespace:
  if(feof(ini)) {
    *top_level = sections;
    return KV_SUCCESS;
  }
  line_num++;
  c = getc(ini);
  if(c == '\n')
    line_num++;
  while(isspace(c)) {
    c = getc(ini);
    if(c == '\n')
      line_num++;
  }

  switch(c) {
    case -1:
    case ';':
    case '#': {
      goto eatcomments;
    } break;

    case ',':
    case ':': 
    case ']':
    case '=': {
      ERROR("INI line %d unexpected %c", line_num, c);
      return KV_PARSE_ERROR;
    } break;

    case '[': {
      goto eatsection;
    } break;

    case '\r':
    case '\n': {
      goto eatwhitespace;
    } break;

    default:
    /* fall out */
    break;
  }

eatkey:
  cur_key = string_new();
  while(1) {
    switch(c) {
      case -1:
      case ';':
      case '#': 
      case '\r':
      case '\n': {
	WARNING("INI line %d ignoring floating key %.*s", line_num, cur_key->len, cur_key->str);
	goto eatcomments;
      } break;

      case ',':
      case ':': 
      case ']':
      case '[': {
	string_free(cur_key);
	ERROR("INI line %d unexpected %c", line_num, c);
	return KV_PARSE_ERROR;
      } break;

      case '=': { 
	c = getc(ini);
	goto eatvalue;
      }

      case ' ':
      case '\t': {
        /* eat whitespace */
      } break;

      default: {
	string_append_char(cur_key, c);
      }
    }
    c = getc(ini);
  }

eatvalue:
  cur_val = string_new();
  last_was_space = 0;
  while(1) {
    switch(c) {
      case -1:
      case ';':
      case '#': 
      case '\r':
      case '\n': {
	kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), kv_tracker_str(tracker,cur_val));
	goto eatcomments;
      } break;

      case ']':
      case '=': 
      case '[': {
	ERROR("INI line %d unexpected %c", line_num, c);
	return KV_PARSE_ERROR;
      } break;

      case ',': {
	goto eatlist;
      } break;

      case '|':
      case ':': {
	goto eatkeyvalue;
      } break;

      case ' ':
      case '\t': {
	/* space separated list? */
	if(cur_val->len > 0) {
	  last_was_space = 1;
	} else {
	  /* eat whitespace */
	}
      } break;

      default: {
	if(!last_was_space)
	  string_append_char(cur_val, c);
	else
	  goto eatlist;
      }
    }
    c = getc(ini);
  }

eatlist:
  kv_list_new(&cur_list);
  kv_tracker_append(tracker, cur_list);
  kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
  cur_val = string_new();
  while(1) {
    switch(c) {
      case -1:
      case ';':
      case '#': 
      case '\r':
      case '\n': {
	if(cur_val->len > 0)
	  kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
	else
	  string_free(cur_val);
	kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), cur_list);
	goto eatcomments;
      } break;

      case ']':
      case '=': 
      case '[': {
	ERROR("INI line %d unexpected %c", line_num, c);
	string_free(cur_val);
	return KV_PARSE_ERROR;
      } break;

      case ' ':
      case '\t': 
      case ',': {
	if(cur_val->len > 0) {
	  kv_list_insert_last(cur_list, kv_tracker_str(tracker,cur_val));
	  cur_val = string_new();
	}
      } break;

      case ':': {
	ERROR("INI line %d unexpected %c", line_num, c);
	return KV_PARSE_ERROR;
      } break;

      default: {
	string_append_char(cur_val, c);
      }
    }
    c = getc(ini);
  }

eatkeyvalue:
  kv_store_new(&cur_store);
  kv_tracker_append(tracker, cur_store);
  cur_store_key = cur_val;
  cur_store_val = NULL;
  cur_val = string_new();
  last_was_space = 0;
  while(1) {
    switch(c) {
      case -1:
      case ';':
      case '#': 
      case '\r':
      case '\n': {
	if(cur_store_key->len > 0 && cur_store_val) {
	  kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	} else {
	  string_free(cur_store_key);
	  if(cur_store_val)
	    string_free(cur_store_val);
	}
	kv_store_set(cur_section, kv_tracker_str(tracker,cur_key), cur_store);
	goto eatcomments;
      } break;

      case ']':
      case '=': 
      case '[': {
	ERROR("INI line %d unexpected %c", line_num, c);
	return KV_PARSE_ERROR;
      } break;

      case ' ':
      case '\t':  {
	last_was_space = 1;
      } break;

      case ',': {
	if(cur_store_key->len > 0 && cur_store_val) {
	  kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	} else {
	  string_free(cur_store_key);
	  if(cur_store_val)
	    string_free(cur_store_val);
	}
	cur_val = string_new();
	cur_store_key = cur_val;
	cur_store_val = NULL;
      } break;

      case '|': 
      case ':': {
	if(!cur_store_val) {
	  cur_val = string_new();
	  cur_store_val = cur_val;
	} else {
	  ERROR("INI line %d unexpected %c key is %.*s val is %.*s", line_num, c, cur_store_key->len, cur_store_key->str, 
	    cur_store_val ? cur_store_val->len : 0, cur_store_val ? cur_store_val->str : NULL);
	  return KV_PARSE_ERROR;
	}
      } break;

      default: {
	if(last_was_space) {
	  if(cur_store_key->len > 0 && cur_store_val && cur_store_val->len) {
	    kv_store_set(cur_store, kv_tracker_str(tracker,cur_store_key), kv_tracker_str(tracker,cur_store_val));
	    cur_val = string_new();
	    cur_store_key = cur_val;
	    cur_store_val = NULL;
	  } 
	}
	string_append_char(cur_val, c);
	last_was_space = 0;
      }
    }
    c = getc(ini);
  }

eatsection:
  cur_store = sections;
  kv_store_new(&cur_section);
  cur_val = string_new();
  c = getc(ini);
  while(1) {
    switch(c) {
      case -1:
      case ';':
      case '#': 
      case '\r':
      case '\n': 
      case '=': 
      case '[': {
	ERROR("INI line %d unexpected %c", line_num, c);
	string_free(cur_val);
	return KV_PARSE_ERROR;
      } break;

      case ']': {
	if(cur_val->len > 0) {
	  string_prepend_char(cur_val, '.');
	  kv_element_t *rtn, str = kve_from_str(cur_val);
	  if(KV_DOES_NOT_EXIST != kv_store_get(cur_store, &str, &rtn)) {
	    kv_store_free(&cur_section);
	    cur_section = rtn;
	  } else {
	    kv_tracker_append(tracker, cur_section);
	    kv_store_set(cur_store, kv_tracker_str(tracker,cur_val), cur_section);
	  }
	} else {
	  kv_store_free(&cur_section);
	  string_free(cur_val);
	}
	goto eatcomments;
      } break;

      case '\t':
      case ' ':
      case '.': 
      case '|': 
      case ':': {
	if(cur_val->len > 0) {
	  string_prepend_char(cur_val, '.');
	  kv_element_t *rtn, str = kve_from_str(cur_val);
	  if(KV_DOES_NOT_EXIST != kv_store_get(cur_store, &str, &rtn)) {
	    kv_store_free(&cur_section);
	    cur_section = rtn;
	  } else {
	    kv_tracker_append(tracker, cur_section);
	    kv_store_set(cur_store, kv_tracker_str(tracker,cur_val), cur_section);
	  }
	  cur_store = cur_section;
	  kv_store_new(&cur_section);
	  cur_val = string_new();
	}
      } break;

      default: {
	string_append_char(cur_val, c);
      }
    }
    c = getc(ini);
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * TESTING
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#if defined(KV_STORE_TEST)

int main(int argc, char *argv[]) {

  kv_element_t tracker;
  kv_tracker_init(&tracker);
  kv_element_t * options;

  kv_from_args(argc-1, argv+1, &tracker, &options);
  kv_element_print(options, 0);

  string_t test_string;
  string_init_from_cstr(&test_string, "this is a string");
  kv_element_t * str = kv_tracker_str(&tracker, &test_string);
  kv_element_t * i = kv_tracker_i64(&tracker, 5);
  kv_element_t * rslt;

  kv_element_t * store; kv_store_new(&store);
  kv_store_set(store, str, i);

  kv_store_print(store, 0);

  kv_element_t * list; kv_list_new(&list);
  kv_tracker_append(&tracker, list);

  kv_list_insert_first(list, str);
  kv_list_insert_first(list, i);
  kv_list_print(list, 0);

  kv_list_pop_first(list, &rslt);
  kv_list_pop_last(list, &rslt);

  kv_list_insert_first(list, str);
  kv_list_insert_first(list, i);
  kv_list_insert_first(list, str);
  kv_list_insert_first(list, i);
  kv_list_insert_last(list, store);

  kv_list_print(list, 0);

  kv_list_pop_last(list, &rslt);

  kv_store_set(store, list, str);
  kv_store_print(store, 0);

  kv_store_set(store, list, i);
  kv_store_print(store, 0);

  printf("**** INI FILE ***\n");
  kv_element_t * inidata;
  FILE * inifile = fopen("data/example.ini", "r");
  kv_from_ini(inifile, &tracker, &inidata);
  kv_store_print(inidata, 0);

  kv_tracker_free_internal(&tracker);
}

#endif
