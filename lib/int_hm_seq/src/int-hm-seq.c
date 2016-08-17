#include "int_hm_seq.h"

#include <stdlib.h>

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

int_hm_seq_t *
int_hm_seq_new(int64_t size) {
  size <<= 1;
  int64_t pow = 16;

  while(pow < size) {
    pow <<= 1;
  }

  int_hm_seq_t * rtn = malloc(sizeof(int_hm_seq_t));
  rtn->size = pow;
  rtn->mask = pow-1;
  rtn->elements = 0;
  rtn->removed = 0;
  rtn->keys = malloc((sizeof(int64_t) + sizeof(int64_t)) * pow);
  rtn->vals = rtn->keys + pow;

  for(uint64_t i = 0; i < rtn->size; i++) {
    rtn->keys[i] = INT_HT_SEQ_EMPTY;
    rtn->vals[i] = INT_HT_SEQ_EMPTY;
  }

  return rtn;
}

int_hm_seq_t *
int_hm_seq_free(int_hm_seq_t * ht) {
  if(ht) {
    free(ht);
  }
  return NULL;
}

int_hm_seq_t * 
int_hm_seq_expand(int_hm_seq_t * ht, int64_t new_size) {
  int64_t pow = ht->size;

  while(pow < new_size) {
    pow <<= 1;
  }

  int_hm_seq_t tmp;
  tmp.size = pow;
  tmp.mask = pow-1;
  tmp.elements = 0;
  tmp.removed = 0;
  tmp.keys = malloc((sizeof(int64_t) + sizeof(int64_t)) * pow);
  tmp.vals = tmp.keys + pow;

  for(uint64_t i = 0; i < tmp.size; i++) {
    tmp.keys[i] = INT_HT_SEQ_EMPTY;
    tmp.vals[i] = INT_HT_SEQ_EMPTY;
  }

  for(uint64_t i = 0; i < ht->size; i++) {
    if(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != INT_HT_SEQ_REMOVED) {
      int_hm_seq_insert(&tmp, ht->keys[i], ht->vals[i]);
    }
  }

  free(ht->keys);
  *ht = tmp;
}

int_hm_seq_t * 
int_hm_seq_expand_versioned(int_hm_seq_t * ht, int64_t new_size, int64_t v) {
  int64_t pow = ht->size;

  while(pow < new_size) {
    pow <<= 1;
  }

  int_hm_seq_t tmp;
  tmp.size = pow;
  tmp.mask = pow-1;
  tmp.elements = 0;
  tmp.removed = 0;
  tmp.keys = malloc((sizeof(int64_t) + sizeof(int64_t)) * pow);
  tmp.vals = tmp.keys + pow;

  for(uint64_t i = 0; i < tmp.size; i++) {
    tmp.keys[i] = INT_HT_SEQ_EMPTY;
    tmp.vals[i] = INT_HT_SEQ_EMPTY;
  }

  for(uint64_t i = 0; i < ht->size; i++) {
    if(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != INT_HT_SEQ_REMOVED && ht->vals[i] == v) {
      int_hm_seq_insert(&tmp, ht->keys[i], ht->vals[i]);
    }
  }

  free(ht->keys);
  *ht = tmp;
}

int_hm_seq_t *
int_hm_seq_insert_versioned(int_hm_seq_t * ht, int64_t k, int64_t v) {
  if(((double)(ht->elements + ht->removed)) > (0.7f) * ((double)ht->size)) {
    int_hm_seq_expand(ht, ht->size * 2);
  }

  int64_t i = mix(k) & ht->mask;
  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != INT_HT_SEQ_REMOVED && ht->keys[i] != k && ht->vals[i] == v) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys[i] != k) {
    if(ht->keys[i] == INT_HT_SEQ_REMOVED) {
      ht->removed--;
    }

    ht->keys[i] = k; ht->vals[i] = v;
    ht->elements++;
  }
  return ht;
}

int_hm_seq_t *
int_hm_seq_insert(int_hm_seq_t * ht, int64_t k, int64_t v) {
  if(((double)(ht->elements + ht->removed)) > (0.7f) * ((double)ht->size)) {
    int_hm_seq_expand(ht, ht->size * 2);
  }

  int64_t removed_index = -1;
  int64_t i = mix(k) & ht->mask;
  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
    if(i == -1 && ht->keys[i] == INT_HT_SEQ_REMOVED) {
      removed_index = i;
    }
  }

  if(removed_index != -1) {
    i = removed_index;
    ht->removed--;
  }

  ht->keys[i] = k; ht->vals[i] = v;
  ht->elements++;
}

int_hm_seq_t *
int_hm_seq_remove(int_hm_seq_t * ht, int64_t k) {
  int64_t i = mix(k) & ht->mask;

  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys[i] == k) {
    ht->elements--;
    ht->keys[i] = INT_HT_SEQ_REMOVED;
    ht->vals[i] = INT_HT_SEQ_EMPTY;
    ht->removed++;
  }
  return ht;
}

int64_t
int_hm_seq_get(int_hm_seq_t * ht, int64_t k) {
  int64_t i = mix(k) & ht->mask;

  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys[i] == k) {
    return ht->vals[i];
  } else {
    return INT_HT_SEQ_EMPTY;
  }
}

int64_t *
int_hm_seq_get_location(int_hm_seq_t * ht, int64_t k) {
  if(((double)(ht->elements + ht->removed)) > (0.7f) * ((double)ht->size)) {
    int_hm_seq_expand(ht, ht->size * 2);
  }

  int64_t i = mix(k) & ht->mask;

  int64_t removed_index = -1;
  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
    if(i == -1 && ht->keys[i] == INT_HT_SEQ_REMOVED) {
      removed_index = i;
    }
  }

  if(removed_index != -1) {
    i = removed_index;
    ht->removed--;
  }

  if(ht->keys[i] != k) {
    ht->elements++;
    ht->keys[i] = k;
  }

  return ht->vals + i;
}

#if defined(INT_HM_SEQ_TEST)
#include <stdio.h>

int main(int argc, char *argv[]) {

  int_hm_seq_t * ht = int_hm_seq_new(10);

  if(ht->size != 32) {
    printf("Initialization failed.\n");
    return -1;
  }
  for(uint64_t i = 1; i < 50; i += 2) {
    int_hm_seq_insert(ht, i, i);
    if(i != int_hm_seq_get(ht, i)) {
      printf("Insertion of %ld failed.\n", i);
      return -1;
    }
  }
  printf("insert test pass\n");

  for(uint64_t i = 0; i < 50; i++) {
    if(i % 2 == 1) {
      if(i != int_hm_seq_get(ht, i)) {
	printf("%ld does not exist, but was inserted.\n", i);
	return -1;
      }
      if(i != *int_hm_seq_get_location(ht, i)) {
	printf("Get location %ld does not exist, but was inserted.\n", i);
	return -1;
      }
    } else {
      if(i == int_hm_seq_get(ht, i)) {
	printf("%ld does exist, but was not inserted.\n", i);
	return -1;
      }
    }
  }
  printf("insert test pass\n");

  int_hm_seq_free(ht);
  ht = int_hm_seq_new(10);

  for(uint64_t i = 0; i < 50; i += 2) {
    *int_hm_seq_get_location(ht, i) = i;
    if(i != int_hm_seq_get(ht, i)) {
      printf("Insertion of %ld failed.\n", i);
      return -1;
    }
  }

  printf("insert test pass\n");

  for(uint64_t i = 0; i < 50; i++) {
    if(i % 2 != 1) {
      if(i != int_hm_seq_get(ht, i)) {
	printf("%ld does not exist, but was inserted.\n", i);
	return -1;
      }
    } else {
      if(i == int_hm_seq_get(ht, i)) {
	printf("%ld does exist, but was not inserted.\n", i);
	return -1;
      }
    }
  }

  printf("insert test pass\n");

  int_hm_seq_free(ht);

  uint64_t count = 100000000;
  ht = int_hm_seq_new(count);
  tic();
  for(uint64_t i = 0; i < count; i++) {
    int_hm_seq_insert(ht, i, i);
  }
  double timecount = toc();
  printf("time for count %ld %lf\n", count, timecount);
  printf("updates per  sedc %lf\n", ((double)count)/ timecount);

  for(uint64_t i = 0; i < count; i++) {
    if(i != int_hm_seq_get(ht, i)) {
      printf("the value does not exist\n");
    }
  }

  int_hm_seq_free(ht);
}
#endif
