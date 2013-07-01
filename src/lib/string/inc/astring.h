#ifndef  ASTRING_H
#define  ASTRING_H

#include <stdint.h>

typedef struct string {
  int max;
  int len;
  char * str;
} string_t;

string_t *
string_new();

string_t *
string_init(string_t * str);

void
string_free_internal(string_t * str);

string_t *
string_free(string_t * str);

string_t *
string_init_from_cstr(string_t * str, char * c);

string_t *
string_new_from_cstr(char * c);

uint64_t
string_hash(string_t * str);

void
string_append_char(string_t * s, char c);

void
string_prepend_char(string_t * s, char c);

void
string_append_string(string_t * dest, string_t * source);

void
string_append_cstr_len(string_t * dest, char * s, int len);

void
string_append_cstr(string_t * dest, char * s);

int
string_is_prefix(string_t * s, string_t * pre);

int
string_equal(string_t * s1, string_t * s2);

void
string_truncate(string_t * s, int len);

int
string_length(string_t * s);

#endif  /*ASTRING_H*/
