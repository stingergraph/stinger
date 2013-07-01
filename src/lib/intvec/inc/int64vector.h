#include <stdlib.h>
#include <inttypes.h>

typedef struct int64vector {
  int64_t * arr;
  int64_t len;
  int64_t max;
} int64vector_t;

int64vector_t *
int64vector_new();

void
int64vector_free(int64vector_t ** s);

int64vector_t *
int64vector_new_from_carr(int64_t * c, int64_t len);

void
int64vector_append_int64_t(int64vector_t * s, int64_t c);

void
int64vector_append_int64vector(int64vector_t * dest, int64vector_t * source);

int
int64vector_is_prefix(int64vector_t * s, int64vector_t * pre);

void
int64vector_truncate(int64vector_t * s, int len);
