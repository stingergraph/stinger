#include "int64vector.h"
#include <stdlib.h>

int64vector_t *
int64vector_new() {
  int64vector_t * rtn = calloc(1,sizeof(int64vector_t));
  rtn->max = 128;
  rtn->len = 0;
  rtn->arr = calloc(rtn->max, rtn->max * sizeof(int64_t));
  return rtn;
}

void
int64vector_free(int64vector_t ** s) {
  if(*s && (*s)->arr)
    free((*s)->arr);
  if(*s)
    free(*s);
  *s = NULL;
}

int64vector_t *
int64vector_new_from_carr(int64_t * c, int64_t len) {
  int64vector_t * rtn = int64vector_new();
  while(len--) {
    int64vector_append_int64_t(rtn, *c);
  }
  return rtn;
}

void
int64vector_append_int64_t(int64vector_t * s, int64_t c) {
  if(s->len >= s->max-1) {
    s->max *= 2;
    s->arr = realloc(s->arr, s->max);
  }
  s->arr[s->len++] = c;
}

void
int64vector_append_int64vector(int64vector_t * dest, int64vector_t * source) {
  while(dest->len + source->len >= dest->max-1) {
    dest->max *= 2;
    dest->arr = realloc(dest->arr, dest->max);
  }
  int64_t * c = source->arr;
  while(c < source->arr + source->len) {
    dest->arr[dest->len++] = *c;
    c++;
  }
}

int
int64vector_is_prefix(int64vector_t * s, int64vector_t * pre) {
  if(pre->len > s->len) {
    return 0;
  }
  int64_t * sc = s->arr;
  int64_t * pc = pre->arr;
  while(pc < pre->arr + pre->len) {
    if(*(sc++) != *(pc++)) {
      return 0;
    }
  }
  return 1;
}

void
int64vector_truncate(int64vector_t * s, int len) {
  s->len = len;
}
