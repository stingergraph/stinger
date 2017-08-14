#ifndef  XML_SUPPORT_H
#define  XML_SUPPORT_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#define XML_TAB "  "

#define XML_INIT(OUT, INDENT) { FILE * xml_out = OUT; int64_t xml_indent = INDENT 
#define XML_PRINT_INDENT() for(uint64_t i = 0; i < xml_indent; i++) fprintf(xml_out, XML_TAB);
#define XML_TAG_START(X)	      XML_PRINT_INDENT();  fprintf(xml_out, "<%s ", #X); xml_indent++;
#define XML_ATTRIBUTE_DOUBLE(NAME,VAL)	      fprintf(xml_out, "%s=\"%lf\" ", #NAME, VAL);
#define XML_ATTRIBUTE_INT64(NAME,VAL)	      fprintf(xml_out, "%s=\"%" PRId64 "\" ", #NAME, VAL);
#define XML_ATTRIBUTE_STRING(NAME,VAL)	      fprintf(xml_out, "\"%s\" : \"%s\",\n", #NAME, VAL);
#define XML_TAG_END()			      fprintf(xml_out, ">\n");
#define XML_TAG_END_AND_CLOSE()	      xml_indent--; fprintf(xml_out, "/>\n");
#define XML_VALUE_DOUBLE(NAME,VAL)	      XML_PRINT_INDENT();  fprintf(xml_out, "%lf\n", #NAME, VAL);
#define XML_VALUE_INT64(NAME,VAL)	      XML_PRINT_INDENT();  fprintf(xml_out, "%ld\n", #NAME, VAL);
#define XML_VALUE_STRING(NAME,VAL)	      XML_PRINT_INDENT();  fprintf(xml_out, "%s\n", #NAME, VAL);
#define XML_TAG_CLOSE(X)	      xml_indent--; XML_PRINT_INDENT();  fprintf(xml_out, "</%s>\n", #X);
#define XML_END() }

void
stinger_vertex_to_xml(const stinger_vertices_t * vertices, vindex_t v, FILE * out, int64_t indent_level);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*XML_SUPPORT_H*/
