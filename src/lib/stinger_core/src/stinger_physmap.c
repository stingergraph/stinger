#include <stdint.h>
#include <string.h>

#include "stinger_physmap.h"
#include "stinger_vertex.h"
#include "stinger.h"
#include "x86_full_empty.h"
#include "xmalloc.h"


/**
* @brief Allocates and initializes a new physical mapping.
*
* @param max_size
*
* @return 
*/
stinger_physmap_t *
stinger_physmap_new(int64_t max_size) {
  return stinger_names_new(max_size);
}

void
stinger_physmap_init(stinger_physmap_t * physmap, int64_t max_size) {
  stinger_names_init(physmap, max_size);
}

size_t
stinger_physmap_size(int64_t max_size) {
  return stinger_names_size(max_size);
}

stinger_physmap_t *
stinger_physmap_free(stinger_physmap_t ** physmap) {
  return stinger_names_free(physmap);
}

/**
 * @brief Gets or creates the mapping between the input byte-string and the vertex 
 * space 0 .. stinger_vertices_max_vertices_get()-1
 *
 * Bytes string can contain null and other special characters.  It is treated as raw data. This function
 * does make a copy of the input string.
 *
 * @param S Pointer to the stinger graph.
 * @param byte_string An arbirary length string of bytes.
 * @param length The length of the string. Must be correct.
 * @param vtx_out The output vertex mapping.
 * @return 0 on existing mapping found. 1 on new mapping created. -1 on failure (STINGER is full).
 */
int
stinger_physmap_mapping_create(stinger_physmap_t * p, stinger_vertices_t * v, const char * byte_string, int64_t length, vindex_t * vtx_out)
{
  return stinger_names_create_type(p, byte_string, vtx_out);
}

/**
 * @brief Get the mapping between the input byte-string and the vertex space 0 .. stinger_vertices_max_vertices_get()-1 if it exists.
 *
 * @param S Pointer to the stinger graph.
 * @param byte_string An arbirary length string of bytes.
 * @param length The length of the string. Must be correct.
 * @retrun The vertex ID or -1 on failure.
 */
vindex_t
stinger_physmap_vtx_lookup(stinger_physmap_t * p, stinger_vertices_t * v, const char * byte_string, int64_t length)
{
  return stinger_names_lookup_type(p, byte_string);
}

/**
 * @brief Get the mapping between the input vertex in the space space 0 .. stinger_vertices_max_vertices_get()-1 and its representative
 *	string if it exists.
 *
 * This function assumes that the input buffer might could already be allocated, but will allocate / reallocate as needed.
 *
 * @param S Pointer to the stinger graph.
 * @param vertexID The vertex from which to get the string.
 * @param outbuffer Output buffer where you would like the data to be placed.
 * @param outbufferlength The length of the output buffer (will be set or increased if allocated / reallocated or if string is shorter).
 * @retrun 0 on success, -1 on failure.
 */
int
stinger_physmap_id_get(stinger_physmap_t * p, stinger_vertices_t * v, vindex_t vertexID, char ** outbuffer, int64_t * outbufferlength)
{
  char * name = stinger_names_lookup_name(p, vertexID);
  if(name) {
    int len = strlen(name);

    if(*outbuffer == NULL || *outbufferlength == 0){
      *outbuffer = xmalloc(len * sizeof(char));
      if(NULL == *outbuffer) {
	return -1;
      }
    } else if(*outbufferlength < len) {
      void * tmp = xrealloc(*outbuffer, len);
      if(tmp) {
	*outbuffer = tmp;
      } else {
	return -1;
      }
    }
    memcpy(*outbuffer, name, len);
    *outbufferlength = len;
    return 0;
  }
  return -1;
}

int
stinger_physmap_id_direct(stinger_physmap_t * p, stinger_vertices_t * v, vindex_t vertexID, char ** out_ptr, int64_t * out_len)
{
  char * name = stinger_names_lookup_name(p, vertexID);
  if(name) {
    *out_len = strlen(name);
    *out_ptr = name;
    return 0;
  }
  return -1;
}

int64_t
stinger_physmap_nv(stinger_physmap_t * p) {
  return stinger_names_count(p);
}

