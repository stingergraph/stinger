#include "stinger.h"
#include "xml_support.h"

#define VTX(v) stinger_vertices_vertex_get(vertices, v)

#define CONST_VTX(v) const_stinger_vertices_vertex_get(vertices, v)


inline void
stinger_vertex_to_xml(const stinger_vertices_t * vertices, vindex_t v, FILE * out, int64_t indent_level)
{
  const stinger_vertex_t * vout = CONST_VTX(v);

  XML_INIT(out, indent_level);
  XML_TAG_START(vertex);
  XML_ATTRIBUTE_VTYPE(type, vout->type);
  XML_ATTRIBUTE_VWEIGHT(weight, vout->weight);
  XML_ATTRIBUTE_INT64(inDegree, vout->inDegree);
  XML_ATTRIBUTE_INT64(outDegree, vout->outDegree);
  XML_TAG_END();
  XML_TAG_START(edges);
  XML_TAG_END();
  XML_TAG_CLOSE(edges);
#if defined(STINGER_VERTEX_KEY_VALUE_STORE)
  /* TODO attributes */
#endif
  XML_TAG_CLOSE(vertex);
  XML_END();
}
