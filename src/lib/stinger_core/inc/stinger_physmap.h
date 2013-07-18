#ifndef  STINGER_PHYSMAP_H
#define  STINGER_PHYSMAP_H

#include <stdio.h>
#include "stinger_names.h"
#include "stinger_vertex.h"


typedef struct physID {
} physID_t;

typedef stinger_names_t stinger_physmap_t;


stinger_physmap_t *
stinger_physmap_new(int64_t max_size);

void
stinger_physmap_init(stinger_physmap_t * physmap, int64_t max_size);

size_t
stinger_physmap_size(int64_t max_size);

void
stinger_physmap_free(stinger_physmap_t ** physmap);

int
stinger_physmap_mapping_create(stinger_physmap_t * p, stinger_vertices_t * v, const char * byte_string, int64_t length, int64_t * vtx_out);

vindex_t
stinger_physmap_vtx_lookup(stinger_physmap_t * p, stinger_vertices_t * v, const char * byte_string, int64_t length);

int
stinger_physmap_id_get(stinger_physmap_t * p, stinger_vertices_t * v, vindex_t vertexID, char ** outbuffer, int64_t * outbufferlength);

int
stinger_physmap_id_direct(stinger_physmap_t * p, stinger_vertices_t * v, vindex_t vertexID, char ** out_ptr, int64_t * out_len);

int64_t
stinger_physmap_nv(stinger_physmap_t * p);


#endif  /*STINGER_PHYSMAP_H*/
