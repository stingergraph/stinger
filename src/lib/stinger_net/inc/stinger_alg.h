#ifndef  STINGER_ALG_H
#define  STINGER_ALG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  int socket_handle;
} stinger_registered_alg;

typedef struct {
  char * name;		       /* required argument */
  char * host;
  int port;
  int64_t data_per_vertex;
  char * data_description;
} stinger_register_alg_params;

#define stinger_register_alg(...) stinger_register_alg_impl((stinger_register_alg_params){__VAR_ARGS__})
stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params);

#ifdef __cplusplus
}
#endif

#endif  /*STINGER_ALG_H*/
