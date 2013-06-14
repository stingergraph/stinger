#ifndef STINGER_DEPRECATED_H
#define STINGER_DEPRECATED_H

#include "stinger-config.h"

#include <stdint.h>

/* These functions are included for backwards compatability, but are not recommended
 * for use.  No guarantees are made about their performance, and they are no longer
 * part of the STINGER API.
 */

int
stinger_remove_and_insert_edges (struct stinger *G,
                                 int64_t type, int64_t from,
                                 int64_t nremove, int64_t * remove,
                                 int64_t ninsert, int64_t * insert,
                                 int64_t * weight, int64_t timestamp);

int64_t
stinger_remove_and_insert_batch (struct stinger * G, int64_t type,
                                 int64_t timestamp, int64_t n,
                                 int64_t * insoff, int64_t * deloff,
                                 int64_t * act);
void
stinger_gather_typed_successors_serial (const struct stinger *G, int64_t type,
                                        int64_t v, size_t * outlen,
                                        int64_t * out, size_t max_outlen);

#define stinger_outdegree(S,Y)	stinger_outdegree_get(S,Y)
#define stinger_indegree(S,Y)	stinger_indegree_get(S,Y)
#define stinger_vtype(S,Y)	stinger_vtype_get(S,Y)

#if defined(STINGER_FORCE_OLD_MAP)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * OLD STINGER_PHYSMAP.H
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MAX_VTXID 0xFFFFF
/** \def MAX_VTXID
*   \brief Maximum number of vertices the physical mapper can support.
*   Note: If the physical mapper produces errors, increase this number.
*/
#define MAX_NODES 0xFFFFF
/** \def MAX_NODES
*   \brief Maximum number internal nodes that the physical mapper can support.
*   Note: If the physical mapper produces errors, increase this number.
*/

typedef struct stinger_physmap stinger_physmap_t;

stinger_physmap_t * 
stinger_physmap_create();

void
stinger_physmap_delete(stinger_physmap_t * map);

uint64_t
stinger_physmap_create_mapping (stinger_physmap_t * map, char * string, uint64_t length);

uint64_t
stinger_physmap_get_mapping (stinger_physmap_t * map, char * string, uint64_t length);

int
stinger_physmap_get_key (stinger_physmap_t * map, char ** outbuffer, uint64_t * outbufferlength, uint64_t vertexID);

#endif  /*STINGER-DEPRECATED_H*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * END STINGER_PHYSMAP.H
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif
