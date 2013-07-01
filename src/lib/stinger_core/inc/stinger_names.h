#ifndef STINGER_NAMES_H_
#define STINGER_NAMES_H_

#define NAME_STR_MAX 255

typedef struct stinger_names {
  int64_t next_string;
  int64_t next_type;
  int64_t max_types;
  int64_t max_names;
  
  int64_t to_name_start;
  int64_t from_name_start;
  int64_t to_int_start;
  uint8_t storage[0];
} stinger_names_t;

stinger_names_t * 
stinger_names_new(int64_t max_types);

stinger_names_t *
stinger_names_free(stinger_names_t ** sn);

int
stinger_names_create_type(stinger_names_t * sn, char * name, int64_t * out);

int64_t
stinger_names_lookup_type(stinger_names_t * sn, char * name);

char *
stinger_names_lookup_name(stinger_names_t * sn, int64_t type);

int64_t
stinger_names_count(stinger_names_t * sn);

#endif
