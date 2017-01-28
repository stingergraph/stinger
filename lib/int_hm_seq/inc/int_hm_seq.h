#ifndef  INT_HM_SEQ_H
#define  INT_HM_SEQ_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>

typedef struct int_hm_seq {
  int64_t size;
  int64_t mask;
  int64_t elements;
  int64_t removed;
  int64_t *keys;
  int64_t *vals;
} int_hm_seq_t;

#define INT_HT_SEQ_EMPTY INT64_MIN
#define INT_HT_SEQ_REMOVED INT64_MAX


int_hm_seq_t *
int_hm_seq_new(int64_t size);

int_hm_seq_t *
int_hm_seq_free(int_hm_seq_t * ht);

void
int_hm_seq_expand(int_hm_seq_t * ht, int64_t new_size);

void
int_hm_seq_expand_versioned(int_hm_seq_t * ht, int64_t new_size, int64_t v);

int_hm_seq_t *
int_hm_seq_insert_versioned(int_hm_seq_t * ht, int64_t k, int64_t v);

void
int_hm_seq_insert(int_hm_seq_t * ht, int64_t k, int64_t v);

int64_t *
int_hm_seq_get_location(int_hm_seq_t * ht, int64_t k);

int_hm_seq_t *
int_hm_seq_remove(int_hm_seq_t * ht, int64_t k);

int64_t
int_hm_seq_get(int_hm_seq_t * ht, int64_t k);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*INT-HM-SEQ_H*/
