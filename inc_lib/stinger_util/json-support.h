#ifndef  JSON_SUPPORT_H
#define  JSON_SUPPORT_H

#include "stinger.h"
#include "astring.h"

#define JSON_TAB "  "

#define JSON_INIT(OUT, INDENT) { FILE * json_out = OUT; int64_t json_indent = INDENT; int not_first = 0;
#define JSON_PRINT_INDENT() for(uint64_t i = 0; i < json_indent; i++) fprintf(json_out, JSON_TAB);
#define JSON_OBJECT_START(X)	      JSON_PRINT_INDENT();  fprintf(json_out, "\"%s\" : {", #X); json_indent++;
#define JSON_OBJECT_START_UNLABELED() JSON_PRINT_INDENT();  fprintf(json_out, "{"); json_indent++;
#define JSON_DOUBLE(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : %lf", not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_INT64(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : %ld", not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_STRING(NAME,VAL)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : \"%s\"", not_first++ ? ",\n" : "\n", #NAME, VAL);
#define JSON_STRING_LEN(NAME,VAL,LEN) JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : \"%.*s\"", not_first++ ? ",\n" : "\n", #NAME, LEN, VAL);
#define JSON_SUBOBJECT(NAME)	      JSON_PRINT_INDENT();  fprintf(json_out, "%s\"%s\" : ", not_first++ ? ",\n" : "\n", #NAME);
#define JSON_OBJECT_END()	      json_indent--; JSON_PRINT_INDENT();  fprintf(json_out, "\n}\n");
#define JSON_END() }

string_t *
egonet_to_json(stinger_t * S, int64_t vtx);

string_t *
group_to_json(stinger_t * S, int64_t * group, int64_t groupsize);

#endif  /*JSON_SUPPORT_H*/
