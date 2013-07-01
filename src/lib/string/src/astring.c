#include "astring.h"
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

string_t *
string_new() {
  string_t * str = calloc(1,sizeof(string_t));
  return string_init(str);
}

string_t *
string_init(string_t * str) {
  str->max = 512;
  str->len = 0;
  str->str = calloc(str->max, sizeof(char));
  return str;
}

void
string_free_internal(string_t * str) {
  if(str->str) free(str->str);
}

string_t *
string_free(string_t * str) {
  if(str) {
    string_free_internal(str);
    free(str);
  }
  return NULL;
}

string_t *
string_init_from_cstr(string_t * str, char * c) {
  string_init(str);
  while(*c != '\0') {
    string_append_char(str, *c);
    c++;
  }
  return str;
}

uint64_t
string_hash(string_t * str) {
  uint64_t hash = 0;
  int64_t i = 0;
  
  {
    union { char * c; int64_t * i; } tmp;
    for(; i < str->len - 7; i += 8) {
      tmp.c = str->str + i;
      hash = hash ^ mix(*tmp.i);
    }
  }
  {
    union { char c[8]; int64_t i; } tmp;
    int64_t j = 0;
    for(; j < str->len - i; j++) {
      tmp.c[j] = str->str[i+j];
    }
    for(; j < 8; j++) {
      tmp.c[j] = 0;
    }
    hash = hash ^ mix(tmp.i);
  }
  return hash;
}

string_t *
string_new_from_cstr(char * c) {
  string_t * str = string_new();
  while(*c != '\0') {
    string_append_char(str, *c);
    c++;
  }
  return str;
}

void
string_append_char(string_t * s, char c) {
  if(s->len >= s->max-1) {
    s->max *= 2;
    s->str = realloc(s->str, s->max);
  }
  s->str[s->len++] = c;
}

void
string_prepend_char(string_t * s, char c) {
  if(s->len >= s->max-1) {
    s->max *= 2;
    s->str = realloc(s->str, s->max);
  }
  int i = s->len++;
  while(i > 0) {
    s->str[i] = s->str[i-1];
    i--;
  }
  s->str[0] = c;
}

void
string_append_string(string_t * dest, string_t * source) {
  while(dest->len + source->len >= dest->max-1) {
    dest->max *= 2;
    dest->str = realloc(dest->str, dest->max);
  }
  char * c = source->str;
  while(c < source->str + source->len) {
    dest->str[dest->len++] = *c;
    c++;
  }
}

void
string_append_cstr_len(string_t * dest, char * s, int len) {
  while(dest->len + len >= dest->max-1) {
    dest->max *= 2;
    dest->str = realloc(dest->str, dest->max);
  }
  char * c = s;
  while(c < s + len) {
    dest->str[dest->len++] = *c;
    c++;
  }
}

void
string_append_cstr(string_t * dest, char * s) {
  int len = 0;
  char * l = s;
  while(*l != '\0') {
    len++; l++;
  }
  while(dest->len + len >= dest->max-1) {
    dest->max *= 2;
    dest->str = realloc(dest->str, dest->max);
  }
  char * c = s;
  while(c < s + len) {
    dest->str[dest->len++] = *c;
    c++;
  }
}

int
string_is_prefix(string_t * s, string_t * pre) {
  if(pre->len > s->len) {
    return 0;
  }
  char * sc = s->str;
  char * pc = pre->str;
  while(pc < pre->str + pre->len) {
    if(*(sc++) != *(pc++)) {
      return 0;
    }
  }
  return 1;
}

int
string_equal(string_t * s1, string_t * s2) {
  if(s1->len != s2->len) {
    return 0;
  }

  char * s1c = s1->str;
  char * s2c = s2->str;
  while(s1c < s1->str + s1->len) {
    if(*(s1c++) != *(s2c++)) {
      return 0;
    }
  }
  return 1;
}

void
string_truncate(string_t * s, int len) {
  s->len = len;
}

int
string_length(string_t * s) {
  return s->len;
}
