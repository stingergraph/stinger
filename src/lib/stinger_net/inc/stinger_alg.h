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
  int64_t result;
  int64_t meta_index;
} stinger_edge_update;

typedef struct {
  int64_t vertex;
  const char * vertex_str;

  int64_t type;
  const char * type_str;

  int64_t set_weight;
  int64_t incr_weight;
  int64_t meta_index;
} stinger_vertex_update;

typedef struct {
  int enabled;
  int map_private;
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
  int64_t num_vertex_updates;
  stinger_vertex_update * vertex_updates;
  int64_t num_metadata;
  uint8_t ** metadata;
  uint64_t * metadata_lengths;
  void * batch_storage;
  batch_type_t batch_type;
} stinger_registered_alg;

typedef struct {
  char * name;		       /* required argument */
  const char * host;
  int port;
  int is_remote;
  int map_private;
  int64_t data_per_vertex;
  char * data_description;
  char ** dependencies;
  int64_t num_dependencies;
} stinger_register_alg_params;

/**
* @brief Register an algorithm to a running STINGER server
*
* @param params All of the configuration parameters for the algorithm.
*
* params.name - [REQUIRED] The unique name identifying the algorithm to the 
* server and other running algorithms.  Note that this name must be less than
* 255 characters and must contain only the characters a-z, A-Z, 0-9, and _.
*
* params.host - [Optional | Default: localhost] The hostname of the machine 
* running the STINGER server.  Note that this must be local unless params.is_remote
* is set.  In the remote mode, batches will be made available, but the graph structure
* itself will not be mapped and cannot be accessed.
*
* params.port - [Optional | Default: 10103] The port on the server that is accepting
* algorithm connection.
*
* params.is_remote - [Optional | Default: false] Whether or not the algorithm is running
* on a remote server from the server.  Note that remote mode will make batches available, but
* not the full graph.
*
* params.data_per_vertex - [Optional | Default: 0] The amount of data stored by the
* algorithm for each vertex in bytes.  Note that although the data is specified in this
* way, the data should be stored in continuous arrays (i.e. if an algorithm stores one
* double and one int64_t per vertex, the storage should contain an NV-length array of 
* doubles and an NV-length array of int64_ts.
* 
* params.data_description - [Required if data_per_vertex is used | Default: NULL]
* A description of the kind of data stored per each vertex (assuming the contiguous C-array
* data layout listed above).  This is used to enable automated parsing and printing of the 
* data.  The first word should contain an automated description following this convention:
*   
*   - f = float
*   - d = double
*   - i = int32_t
*   - l = int64_t
*   - b = byte
*
* 64-bit values and bytes are encouraged unless there is good reason to use 32-bit. All
* words following the first should be identifiers for the coresponding arrays. For example:
*
* dll mean kcore neighbors
*
* An algorithm that finds the largest kcore to which each vertex belongs might use this
* description to store the average size of kcore for each vertex over time, the current kcore 
* size, and the number of neighbors that have the same size.
*
* params.dependencies - [Optional | Default: NULL] The names of the algorithms whose shared
* data is required for this algorithm.  The shared data of these algorithms, the descriptions
* of the data, and the data per vertex sizes will be obtained and automatically mapped.  
* Additionally, specified algorithms are guaranteed to complete the preprocessing phase before
* this algorithm begins preprocessing (and similar for postprocessing).  These algorithms must
* be running on the same server.
*
* params.num_dependencies - [Optional | Default: 0] The count of the number of dependencies
* in params.dependencies.
*
* @return A registered algorithm which contains all of the state information for the algorithm
* and the access points for the batches as they arrive.
*/
stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params);
#define stinger_register_alg(...) stinger_register_alg_impl((stinger_register_alg_params){__VA_ARGS__})

/**
* @brief Request to begin the static initialization phase.
*
* This is a blocking call that sends a request to the server to obtain the next batch of
* updates.  When this returns, the next batch of updates has been obtained and is currently
* available through these members of the algs struct:
*
*  int64_t batch;
*  int64_t num_insertions;
*  stinger_edge_update * insertions;
*  int64_t num_deletions;
*  stinger_edge_update * deletions;
*  void * batch_storage;
*  batch_type_t batch_type;
*
* Between the time that this returns and the time that *end_init is called, the STINGER 
* structure is guaranteed to be static. TODO XXX UNFINISHED DOC HERE
*
* @param alg
*
* @return 
*/
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
