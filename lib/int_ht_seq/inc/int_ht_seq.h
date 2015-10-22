#ifndef  INT_HT_SEQ_H
#define  INT_HT_SEQ_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>

typedef struct int_ht_seq {
  int64_t size;
  int64_t mask;
  int64_t elements;
  int64_t *keys;
} int_ht_seq_t;

#define INT_HT_SEQ_EMPTY INT64_MIN
#define INT_HT_SEQ_DELETED INT64_MAX


int_ht_seq_t *
int_ht_seq_new(int64_t size);

int_ht_seq_t *
int_ht_seq_init(int_ht_seq_t * ht, int64_t size);

int_ht_seq_t *
int_ht_seq_empty(int_ht_seq_t * ht);

int_ht_seq_t *
int_ht_seq_free_internal(int_ht_seq_t * ht);

int_ht_seq_t *
int_ht_seq_free(int_ht_seq_t * ht);

int_ht_seq_t * 
int_ht_seq_expand(int_ht_seq_t * ht, int64_t new_size);

int_ht_seq_t *
int_ht_seq_insert(int_ht_seq_t * ht, int64_t k);

int
int_ht_seq_exists(int_ht_seq_t * ht, int64_t k);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*INT-HT-SEQ_H*/
