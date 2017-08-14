#ifndef  VTX_SET_H
#define  VTX_SET_H

#include <stdint.h>

typedef struct vtx_set {
  char name[64];
  int64_t size;
  int64_t mask;
  int64_t elements;
  int64_t removed;
  int64_t keys_and_vals[0];
} vtx_set_t;

#define INT_HT_SEQ_EMPTY INT64_MIN
#define INT_HT_SEQ_REMOVED INT64_MAX


vtx_set_t *
vtx_set_new(int64_t size, char * name);

vtx_set_t *
vtx_set_free(vtx_set_t * ht);

void
vtx_set_expand(vtx_set_t ** ht, int64_t new_size);

void
vtx_set_insert(vtx_set_t ** ht, int64_t k, int64_t v);

int64_t *
vtx_set_get_location(vtx_set_t ** ht, int64_t k);

vtx_set_t *
vtx_set_remove(vtx_set_t * ht, int64_t k);

int64_t
vtx_set_get(vtx_set_t * ht, int64_t k);

#endif  /*INT-HM-SEQ_H*/
