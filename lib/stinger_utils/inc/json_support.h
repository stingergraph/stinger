#ifndef  JSON_SUPPORT_H
#define  JSON_SUPPORT_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger_core/stinger.h"
#include "string/astring.h"

#define JSON_TAB "  "

#define JSON_INIT(OUT, INDENT) { FILE * json_out = OUT; int64_t json_indent = INDENT; int not_first = 0;
#define JSON_PRINT_INDENT() for(uint64_t i = 0; i < json_indent; i++) fprintf(json_out, JSON_TAB);
#define JSON_OBJECT_START(X)	      JSON_PRINT_INDENT();  fprintf(json_out, "\"%s\" : {", #X); json_indent++;
#define JSON_OBJECT_START_UNLABELED() JSON_PRINT_INDENT();  fprintf(json_out, "{"); json_indent++;
#define JSON_DOUBLE(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : %lf", not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_INT64(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : %" PRId64, not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_STRING(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : \"%s\"", not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_STRING_LEN(NAME,VAL,LEN) JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : \"%.*s\"", not_first++ ? ",\n" : "\n", #NAME, LEN, VAL);
#define JSON_SUBOBJECT(NAME)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : ", not_first++ ? ",\n" : "\n", #NAME);
#define JSON_OBJECT_END()	      json_indent--; JSON_PRINT_INDENT();  fprintf(json_out, "\n}\n");
#define JSON_END() }

void
stinger_physmap_id_to_json(const stinger_physmap_t * p, vindex_t v, FILE * out, int64_t indent_level);

void
stinger_vertex_to_json(const stinger_vertices_t * vertices, stinger_physmap_t * phys, vindex_t v, FILE * out, int64_t indent_level);

void
stinger_vertex_to_json_with_type_strings(const stinger_vertices_t * vertices, const stinger_names_t * tn, stinger_physmap_t * phys, vindex_t v, FILE * out, int64_t indent_level);

string_t *
egonet_to_json(stinger_t * S, int64_t vtx);

string_t *
group_to_json(stinger_t * S, int64_t * group, int64_t groupsize);

string_t *
labeled_subgraph_to_json(stinger_t * S, int64_t src, int64_t * labels, const int64_t vtxlimit);

int
load_json_graph (struct stinger * S, const char * filename);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*JSON_SUPPORT_H*/
