#include "stinger-vertex.h"
#include "stinger.h"
#include "x86-full-empty.h"
#include "xmalloc.h"
#include "json-support.h"

#include <string.h>

#if !defined(STINGER_FORCE_OLD_MAP)

/**
* @brief Allocates and initializes a new physical mapping.
*
* @param max_size
*
* @return 
*/
stinger_physmap_t *
stinger_physmap_new(int64_t max_size) {
  stinger_physmap_t * rtn = xmalloc(sizeof(stinger_physmap_t) + max_size);
  rtn->size = max_size;
  rtn->top = 1;
  return rtn;
}

void
stinger_physmap_free(stinger_physmap_t ** physmap) {
  if(*physmap) {
    free(*physmap);
  }
  *physmap = NULL;
}

uint64_t
xor_hash(uint8_t * byte_string, int64_t length) {
  if(length < 0) length = 0;

  uint64_t out = 0;
  uint64_t * byte64 = (uint64_t *)byte_string;
  while(length > 8) {
    length -= 8;
    out ^= *(byte64);
    byte64++;
  }
  if(length > 0) {
    uint64_t last = 0;
    uint8_t * cur = (uint8_t *)&last;
    uint8_t * byte8 = (uint8_t *)byte64;
    while(length > 0) {
      length--;
      *(cur++) = *(byte8++);
    }
    out ^= last;
  }

  /* bob jenkin's 64-bit mix */
  out = (~out) + (out << 21); // out = (out << 21) - out - 1;
  out = out ^ (out >> 24);
  out = (out + (out << 3)) + (out << 8); // out * 265
  out = out ^ (out >> 14);
  out = (out + (out << 2)) + (out << 4); // out * 21
  out = out ^ (out >> 28);
  out = out + (out << 31);

  return out;
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
#define physIndex   (stinger_vertex_physmap_pointer_get(v,vtx)->index)
#define physLength  (stinger_vertex_physmap_pointer_get(v,vtx)->length)
#define physBuffer  (p->storage)
#define physTop	    (p->top)
  if(length < 0) length = 0;

  uint64_t vtx = xor_hash(byte_string, length);
  vindex_t max = stinger_vertices_max_vertices_get(v);
  vtx = vtx % max; 
  vindex_t init_index = vtx;
  while(1) {
    if(0 == readff((uint64_t *)&physIndex)) {
      vindex_t original = readfe((uint64_t *)&physIndex);
      if(original) {
	if(length == physLength &&
	    bcmp(byte_string,&(physBuffer[original]), length) == 0) {
	  writeef((uint64_t *)&physIndex, original);
	  *vtx_out = vtx;
	  return 0;
	}
      } else {
	vindex_t place = stinger_int64_fetch_add(&(physTop), length);
	if(place + length > p->size) {
	  abort();
	}
	char * physID = &(physBuffer[place]);
	memcpy(physID, byte_string, length);
	physLength = length;
	writeef((uint64_t *)&physIndex, (uint64_t)place);
	*vtx_out = vtx;
	return 1;
      }
      writeef((uint64_t *)&physIndex, original);
    } else if(length == physLength &&
	bcmp(byte_string, &(physBuffer[readff((uint64_t *)&physIndex)]), length) == 0) {
      *vtx_out = vtx;
	return 1;
      return 0;
    }
    vtx++;
    vtx = vtx % max; 
    if(vtx == init_index) 
      return -1;
  }
#undef physIndex
#undef physLength
#undef physBuffer
#undef physTop
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
#define physIndex   (stinger_vertex_physmap_pointer_get(v,vtx)->index)
#define physLength  (stinger_vertex_physmap_pointer_get(v,vtx)->length)
#define physBuffer  (p->storage)
  if(length < 0) length = 0;

  uint64_t vtx = xor_hash(byte_string, length);
  vindex_t max = stinger_vertices_max_vertices_get(v);
  vtx = vtx % max; 
  uint64_t init_index = vtx;
  while(1) {
    if(0 == readff((uint64_t *)&physIndex)) {
      return -1;
    } else if(length == physLength &&
	bcmp(byte_string, &(physBuffer[readff((uint64_t *)&physIndex)]), length) == 0) {
      return vtx;
    }
    vtx++;
    vtx = vtx % max; 
    if(vtx == init_index) 
      return -1;
  }
#undef physIndex
#undef physLength
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
#define physIndex   (stinger_vertex_physmap_pointer_get(v,vertexID)->index)
#define physLength  (stinger_vertex_physmap_pointer_get(v,vertexID)->length)
#define physBuffer  (p->storage)
  if(0 == readff((uint64_t *)&physIndex)) {
    return -1;
  } else {
    if(*outbuffer == NULL || *outbufferlength == 0){
      *outbuffer = xmalloc(physLength * sizeof(char));
      if(NULL == *outbuffer) {
	return -1;
      }
    } else if(*outbufferlength < physLength) {
      void * tmp = xrealloc(*outbuffer, physLength);
      if(tmp) {
	*outbuffer = tmp;
      } else {
	return -1;
      }
    }
    memcpy(*outbuffer, &(physBuffer[readff((uint64_t *)&physIndex)]), physLength);
    *outbufferlength = physLength;
    return 0;
  }
#undef physIndex
#undef physLength
#undef physBuffer
}

int
stinger_physmap_id_direct(stinger_physmap_t * p, stinger_vertices_t * v, vindex_t vertexID, char ** out_ptr, int64_t * out_len)
{
#define physIndex   (stinger_vertex_physmap_pointer_get(v,vertexID)->index)
#define physLength  (stinger_vertex_physmap_pointer_get(v,vertexID)->length)
#define physBuffer  (p->storage)
  *out_ptr = &physBuffer[readff((uint64_t *)&physIndex)];
  *out_len = physLength;
  if(out_ptr)
    return 0;

  *out_len = 0;
  return -1;
#undef physIndex
#undef physLength
#undef physBuffer
}

void
stinger_physmap_id_to_json(const stinger_physmap_t * p, const physID_t * id, FILE * out, int64_t indent_level) {
  JSON_INIT(out, indent_level);
  JSON_OBJECT_START_UNLABELED();
  JSON_STRING_LEN(id, p->storage + readff((uint64_t *)&id->index), id->length);
  JSON_INT64(len, id->length);
  JSON_OBJECT_END();
  JSON_END();
}

#endif
