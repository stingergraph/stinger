#ifndef  STINGER_ALG_H
#define  STINGER_ALG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "stinger_core/stinger.h"

typedef struct {
  stinger_t * stinger;
  char stinger_loc[256];
  char alg_data_loc[256];
  void * alg_data;
  int64_t batch;
  int sock;
  int64_t dep_count;
  char ** dep_name;
  char ** dep_location;
  void ** dep_data;
  char ** dep_description;
  int64_t * dep_data_per_vertex;
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

#define stinger_register_alg(...) stinger_register_alg_impl((stinger_register_alg_params){__VAR_ARGS__})
stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params);

#ifdef __cplusplus
}
#endif

#endif  /*STINGER_ALG_H*/
