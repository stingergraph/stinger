#ifndef  STINGER_ALG_H
#define  STINGER_ALG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "stinger_core/stinger.h"

typedef enum {
  BATCH_STRINGS_ONLY,
  BATCH_NUMBERS_ONLY,
  BATCH_MIXED
} batch_type_t;

typedef struct {
  int64_t type;
  const char * type_str;

  int64_t source;
  const char * source_str;

  int64_t destination;
  const char * destination_str;

  int64_t weight;
  int64_t time;
} stinger_edge_update;

typedef struct {
  int enabled;
  int sock;
  stinger_t * stinger;
  char stinger_loc[256];

  char alg_name[256];
  int64_t alg_num;
  char alg_data_loc[256];
  void * alg_data;
  int64_t alg_data_per_vertex;

  int64_t dep_count;
  char ** dep_name;
  char ** dep_location;
  void ** dep_data;
  char ** dep_description;
  int64_t * dep_data_per_vertex;

  int64_t batch;
  int64_t num_insertions;
  stinger_edge_update * insertions;
  int64_t num_deletions;
  stinger_edge_update * deletions;
  void * batch_storage;
  batch_type_t batch_type;
} stinger_registered_alg;

typedef struct {
  char * name;		       /* required argument */
  char * host;
  int port;
  int64_t data_per_vertex;
  char * data_description;
  char ** dependencies;
  int64_t num_dependencies;
} stinger_register_alg_params;

#define stinger_register_alg(...) stinger_register_alg_impl((stinger_register_alg_params){__VA_ARGS__})
stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params);

stinger_registered_alg *
stinger_alg_begin_init(stinger_registered_alg * alg);

stinger_registered_alg *
stinger_alg_end_init(stinger_registered_alg * alg);

stinger_registered_alg *
stinger_alg_begin_pre(stinger_registered_alg * alg);

stinger_registered_alg *
stinger_alg_end_pre(stinger_registered_alg * alg);

stinger_registered_alg *
stinger_alg_begin_post(stinger_registered_alg * alg);

stinger_registered_alg *
stinger_alg_end_post(stinger_registered_alg * alg);

#ifdef __cplusplus
}
#endif

#endif  /*STINGER_ALG_H*/
