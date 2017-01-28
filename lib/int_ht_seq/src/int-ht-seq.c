#include "int_ht_seq.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

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

int_ht_seq_t *
int_ht_seq_new(int64_t size) {
  int_ht_seq_t * rtn = malloc(sizeof(int_ht_seq_t));

  return int_ht_seq_init(rtn, size);
}

int_ht_seq_t *
int_ht_seq_init(int_ht_seq_t * ht, int64_t size) {
  size <<= 1;
  int64_t pow = 16;

  while(pow < size) {
    pow <<= 1;
  }

  ht->size = pow;
  ht->mask = pow-1;
  ht->elements = 0;
  ht->keys = malloc(sizeof(int64_t) * pow);

  for(uint64_t i = 0; i < ht->size; i++) {
    ht->keys[i] = INT_HT_SEQ_EMPTY;
  }

  return ht;
}

int_ht_seq_t *
int_ht_seq_empty(int_ht_seq_t * ht) {
  for(uint64_t i = 0; i < ht->size; i++) {
    ht->keys[i] = INT_HT_SEQ_EMPTY;
  }
  ht->elements = 0;
  return ht;
}

int_ht_seq_t *
int_ht_seq_free_internal(int_ht_seq_t * ht) {
  if(ht->keys) {
    free(ht->keys);
  }
  return ht;
}

int_ht_seq_t *
int_ht_seq_free(int_ht_seq_t * ht) {
  if(ht) {
    int_ht_seq_free_internal(ht);
    free(ht);
  }
  return NULL;
}

void
int_ht_seq_expand(int_ht_seq_t * ht, int64_t new_size) {
  int_ht_seq_t tmp;

  int_ht_seq_init(&tmp, new_size);

  for(uint64_t i = 0; i < ht->size; i++) {
    if(ht->keys[i] != INT_HT_SEQ_EMPTY) {
      int_ht_seq_insert(&tmp, ht->keys[i]);
    }
  }

  free(ht->keys);
  *ht = tmp;
}

int_ht_seq_t *
int_ht_seq_insert(int_ht_seq_t * ht, int64_t k) {
  if(((double)ht->elements) > (0.7f) * ((double)ht->size)) {
    int_ht_seq_expand(ht, ht->size);
  }

  int64_t i = mix(k) & ht->mask;
  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys[i] == INT_HT_SEQ_EMPTY) {
    ht->keys[i] = k;
    ht->elements++;
  }

  return ht;
}

int
int_ht_seq_exists(int_ht_seq_t * ht, int64_t k) {
  int64_t i = mix(k) & ht->mask;

  while(ht->keys[i] != INT_HT_SEQ_EMPTY && ht->keys[i] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys[i] == k) {
    return 1;
  } else {
    return 0;
  }
}

void
int_ht_seq_print_contents(int_ht_seq_t * ht) {
  printf("[");
  int first = 1;
  for(uint64_t i = 0; i < ht->size; i++) {
    if(ht->keys[i] != INT_HT_SEQ_EMPTY) {
      printf("%s%" PRId64, first ? "" : ", ", ht->keys[i]);
      first = 0;
    }
  }
  printf("]\n");
}

#if defined(INT_HT_SEQ_TEST)
int main(int argc, char *argv[]) {

  int_ht_seq_t * ht = int_ht_seq_new(10);

  if(ht->size != 32) {
    printf("Initialization failed.\n");
    return -1;
  }
  for(uint64_t i = 1; i < 50; i += 2) {
    int_ht_seq_insert(ht, i);
    if(!int_ht_seq_exists(ht, i)) {
      printf("Insertion of %ld failed.\n", i);
      return -1;
    }
  }

  for(uint64_t i = 0; i < 50; i++) {
    if(i % 2 == 1) {
      if(!int_ht_seq_exists(ht, i)) {
	printf("%ld does not exist, but was inserted.\n", i);
	return -1;
      }
    } else {
      if(int_ht_seq_exists(ht, i)) {
	printf("%ld does exist, but was not inserted.\n", i);
	return -1;
      }
    }
  }

  int_ht_seq_empty(ht);

  for(uint64_t i = 0; i < 50; i += 2) {
    int_ht_seq_insert(ht, i);
    if(!int_ht_seq_exists(ht, i)) {
      printf("Insertion of %ld failed.\n", i);
      return -1;
    }
  }

  for(uint64_t i = 0; i < 50; i++) {
    if(i % 2 != 1) {
      if(!int_ht_seq_exists(ht, i)) {
	printf("%ld does not exist, but was inserted.\n", i);
	return -1;
      }
    } else {
      if(int_ht_seq_exists(ht, i)) {
	printf("%ld does exist, but was not inserted.\n", i);
	return -1;
      }
    }
  }

  int_ht_seq_free(ht);

  ht = int_ht_seq_new(1);

  uint64_t count = 100000000;
  tic();
  for(uint64_t i = 0; i < count; i++) {
    int_ht_seq_insert(ht, i);
  }
  double timecount = toc();
  printf("time for count %ld %lf\n", count, timecount);
  printf("updates per  sedc %lf\n", ((double)count)/ timecount);

  for(uint64_t i = 0; i < count; i++) {
    if(!int_ht_seq_exists(ht, i)) {
      printf("the value does not exist\n");
    }
  }

  int_ht_seq_free(ht);
}
#endif
