#include "json_support.h"

#include "stinger_core/xmalloc.h"
#include "fmemopen/fmemopen.h"
#include "int_hm_seq/int_hm_seq.h"

#define VTX(v) stinger_vertices_vertex_get(vertices, v)

void
stinger_physmap_id_to_json(const stinger_physmap_t * p, vindex_t v, FILE * out, int64_t indent_level) {
  JSON_INIT(out, indent_level);
  JSON_OBJECT_START_UNLABELED();
  JSON_STRING(id, stinger_names_lookup_name(p, v));
  JSON_OBJECT_END();
  JSON_END();
}


inline void
stinger_vertex_to_json(const stinger_vertices_t * vertices, stinger_physmap_t * phys, vindex_t v, FILE * out, int64_t indent_level) {
  const stinger_vertex_t * vout = VTX(v);

  JSON_INIT(out, indent_level);
  JSON_OBJECT_START_UNLABELED();
  JSON_INT64(vid, v);
  JSON_VTYPE(vtype, vout->type);
  JSON_VWEIGHT(vweight, vout->weight);
  JSON_INT64(inDegree, vout->inDegree);
  JSON_INT64(outDegree, vout->outDegree);
  JSON_SUBOBJECT(physID);
  stinger_physmap_id_to_json(phys, v, out, indent_level+1);
#if defined(STINGER_VERTEX_KEY_VALUE_STORE)
  /* TODO attributes */
#endif
  JSON_OBJECT_END();
  JSON_END();
}

inline void
stinger_vertex_to_json_with_type_strings(const stinger_vertices_t * vertices, const stinger_names_t * tn, stinger_physmap_t * phys, vindex_t v, FILE * out, int64_t indent_level) {
  const stinger_vertex_t * vout = VTX(v);

  JSON_INIT(out, indent_level);
  JSON_OBJECT_START_UNLABELED();
  JSON_INT64(vid, v);
  char * vtype = stinger_names_lookup_name(tn,vout->type);
  if(vtype) {
    JSON_STRING(vtype, vtype);
  } else {
    JSON_INT64(vtype, vout->type);
  }
  JSON_VWEIGHT(vweight, vout->weight);
  JSON_INT64(inDegree, vout->inDegree);
  JSON_INT64(outDegree, vout->outDegree);
  JSON_SUBOBJECT(physID);
  stinger_physmap_id_to_json(phys, v, out, indent_level+1);
#if defined(STINGER_VERTEX_KEY_VALUE_STORE)
  /* TODO attributes */
#endif
  JSON_OBJECT_END();
  JSON_END();
}

/* produces the egoent of the stinger vertex ID given in JSON form */
string_t *
egonet_to_json(stinger_t * S, int64_t vtx) {
  string_t vertices, edges;
  string_init_from_cstr(&vertices, "\"vertices\" : [ \n");
  string_init_from_cstr(&edges, "\"edges\" : [ \n");

  char vtx_str[1UL<<20UL];
  FILE * vtx_file = fmemopen(vtx_str, sizeof(vtx_str), "w");
  char edge_str[1UL<<20UL];
  int edge_added = 0;

  stinger_vertex_to_json_with_type_strings(stinger_vertices_get(S), stinger_vtype_names_get(S), stinger_physmap_get(S), vtx, vtx_file, 2);
  fflush(vtx_file);
  string_append_cstr(&vertices, vtx_str);
  fseek(vtx_file, 0, SEEK_SET);

  int_hm_seq_t * neighbors = int_hm_seq_new(stinger_outdegree_get(S, vtx));
  int64_t which = 1;

  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, vtx) {

    int64_t source = int_hm_seq_get(neighbors, STINGER_EDGE_DEST);
    if(INT_HT_SEQ_EMPTY == source) {
      source = which++;
      int_hm_seq_insert(neighbors, STINGER_EDGE_DEST, source);
      fprintf(vtx_file, ",\n");
      stinger_vertex_to_json_with_type_strings(stinger_vertices_get(S), stinger_vtype_names_get(S), stinger_physmap_get(S), STINGER_EDGE_DEST, vtx_file, 2);
      fputc('\0',vtx_file);
      fflush(vtx_file);
      string_append_cstr(&vertices, vtx_str);
      fseek(vtx_file, 0, SEEK_SET);
    }

    char * etype = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
    if(!edge_added) {
      edge_added = 1;
      if(etype) {
	sprintf(edge_str, "{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	    etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, (long int) 0, source);
      } else {
	sprintf(edge_str, "{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	    STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, (long int) 0, source);
      }
    } else {
      if(etype) {
	sprintf(edge_str, ",\n{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	    etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, (long int) 0, source);
      } else {
	sprintf(edge_str, ",\n{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	    STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, (long int) 0, source);
      }
    }

    string_append_cstr(&edges, edge_str);
    uint64_t dest = STINGER_EDGE_DEST;

    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, dest) {

      int64_t target = int_hm_seq_get(neighbors, STINGER_EDGE_DEST);
      if(INT_HT_SEQ_EMPTY != target) {
	char * etype = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	if(etype) {
	  sprintf(edge_str, ",\n{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, source, target);
	} else {
	  sprintf(edge_str, ",\n{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, source, target);
	}
	string_append_cstr(&edges, edge_str);
      }

    } STINGER_FORALL_EDGES_OF_VTX_END();

  } STINGER_FORALL_EDGES_OF_VTX_END();

  string_append_cstr(&vertices, "\n], ");
  string_append_cstr(&edges, "\n]\n} ");

  string_t * egonet = string_new_from_cstr("{\n\t");
  string_append_string(egonet, &vertices);
  string_append_string(egonet, &edges);

  int_hm_seq_free(neighbors);
  fclose(vtx_file);
  string_free_internal(&vertices);
  string_free_internal(&edges);
  return egonet;
}

/* produces the union of the egonets of the stinger vertex IDs in
   the group array in JSON form */
string_t *
group_to_json(stinger_t * S, int64_t * group, int64_t groupsize) {
  string_t vertices, edges;
  string_init_from_cstr(&vertices, "\"vertices\" : [ \n");
  string_init_from_cstr(&edges, "\"edges\" : [ \n");

  char vtx_str[1UL<<20UL];
  FILE * vtx_file = fmemopen(vtx_str, sizeof(vtx_str), "w");
  char edge_str[1UL<<20UL];
  int edge_added = 0;

  int64_t which = 0;
  int first_vtx = 1;
  int_hm_seq_t * neighbors = int_hm_seq_new(groupsize ? stinger_outdegree_get(S, group[0]) : 1);

  for(int64_t v = 0; v < groupsize; v++) {
    int64_t group_vtx = int_hm_seq_get(neighbors, group[v]);
    if(INT_HT_SEQ_EMPTY == group_vtx) {
      group_vtx = which++;
      int_hm_seq_insert(neighbors, group[v], group_vtx);
      if(!first_vtx) {
	fprintf(vtx_file, ",\n");
      } else {
	first_vtx = 0;
      }
      stinger_vertex_to_json_with_type_strings(stinger_vertices_get(S), stinger_vtype_names_get(S), stinger_physmap_get(S), group[v], vtx_file, 2);
      fputc('\0',vtx_file);
      fflush(vtx_file);
      string_append_cstr(&vertices, vtx_str);
      fseek(vtx_file, 0, SEEK_SET);
    }

    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, group[v]) {
      int64_t source = int_hm_seq_get(neighbors, STINGER_EDGE_DEST);
      if(INT_HT_SEQ_EMPTY == source) {
	source = which++;
	int_hm_seq_insert(neighbors, STINGER_EDGE_DEST, source);
	fprintf(vtx_file, ",\n");
	stinger_vertex_to_json_with_type_strings(stinger_vertices_get(S), stinger_vtype_names_get(S), stinger_physmap_get(S), STINGER_EDGE_DEST, vtx_file, 2);
	fputc('\0',vtx_file);
	fflush(vtx_file);
	string_append_cstr(&vertices, vtx_str);
	fseek(vtx_file, 0, SEEK_SET);
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  for(int64_t v = 0; v < neighbors->size; v++) {
    int64_t cur = neighbors->keys[v];
    if(cur != INT_HT_SEQ_EMPTY) {
      int64_t cur_val = neighbors->vals[v];
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, cur) {
	if(int_hm_seq_get(neighbors, STINGER_EDGE_DEST) != INT_HT_SEQ_EMPTY && STINGER_EDGE_SOURCE < STINGER_EDGE_DEST) {
	  char * etype = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	  if(!edge_added) {
	    edge_added = 1;
	    if(etype) {
	      sprintf(edge_str, "{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
		  etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, cur_val, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	    } else {
	      sprintf(edge_str, "{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
		  STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, cur_val, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	    }
	  } else {
	    if(etype) {
	      sprintf(edge_str, ",\n{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
		  etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, cur_val, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	    } else {
	      sprintf(edge_str, ",\n{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
		  STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, cur_val, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	    }
	  }
	  string_append_cstr(&edges, edge_str);
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }
  }

  string_append_cstr(&vertices, "\n], ");
  string_append_cstr(&edges, "\n]\n} ");

  string_t * group_str = string_new_from_cstr("{\n\t");
  string_append_string(group_str, &vertices);
  string_append_string(group_str, &edges);

  printf("GROUP OF VERTICES (");
  for(int64_t v = 0; v < groupsize; v++) {
    printf("%ld,", group[v]);
  }
  printf("):\n%s", group_str->str);

  int_hm_seq_free(neighbors);
  fclose(vtx_file);
  string_free_internal(&vertices);
  string_free_internal(&edges);
  return group_str;
}

/* produces the subgraph starting at vtx and including all neighboring
 * vertices with the same label */
string_t *
labeled_subgraph_to_json(stinger_t * S, int64_t src, int64_t * labels, const int64_t vtxlimit_in) {
  const int64_t vtxlimit = (vtxlimit_in < 1 || vtxlimit_in > S->max_nv ? S->max_nv : vtxlimit_in);
  string_t vertices, edges;
  string_init_from_cstr(&vertices, "\"vertices\" : [ \n");
  string_init_from_cstr(&edges, "\"edges\" : [ \n");

  int_hm_seq_t * neighbors = int_hm_seq_new(stinger_outdegree_get(S, src));

  char *vtx_str, *edge_str;
  FILE * vtx_file;

  int64_t which = 0;

  vtx_str = xmalloc (2UL<<20UL);
  edge_str = &vtx_str[1UL<<20UL];
  vtx_file = fmemopen(vtx_str, sizeof(vtx_str) * 1UL<<20UL, "w");

  uint8_t * found = xcalloc(sizeof(uint8_t), S->max_nv);
  int64_t * queue = xmalloc(sizeof(int64_t) * (vtxlimit < 1? 1 : vtxlimit));

  queue[0] = src;
  found[src] = 1;
  int64_t q_top = 1;

  int first_vtx = 1;
  int edge_added = 0;

  for(int64_t q_cur = 0; q_cur < q_top; q_cur++) {
    int64_t group_vtx = which++;
    int_hm_seq_insert(neighbors, queue[q_cur], group_vtx);
    if(!first_vtx) {
      fprintf(vtx_file, ",\n");
    } else {
      first_vtx = 0;
    }
    stinger_vertex_to_json_with_type_strings(stinger_vertices_get(S), stinger_vtype_names_get(S), stinger_physmap_get(S), queue[q_cur], vtx_file, 2);
    fputc('\0',vtx_file);
    fflush(vtx_file);
    string_append_cstr(&vertices, vtx_str);
    fseek(vtx_file, 0, SEEK_SET);

    found[queue[q_cur]] = 2;

    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, queue[q_cur]) {
      if(found[STINGER_EDGE_DEST] > 1) {
	char * etype = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	if(!edge_added) {
	  edge_added = 1;
	  if(etype) {
	    sprintf(edge_str, "{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, group_vtx, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	  } else {
	    sprintf(edge_str, "{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, group_vtx, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	  }
	} else {
	  if(etype) {
	    sprintf(edge_str, ",\n{ \"type\" : \"%s\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      etype, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, group_vtx, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	  } else {
	    sprintf(edge_str, ",\n{ \"type\" : \"%ld\", \"src\" : %ld, \"dest\" : %ld, \"weight\" : %ld, \"time1\" : %ld, \"time2\" : %ld, \"source\" : %ld, \"target\": %ld }", 
	      STINGER_EDGE_TYPE, STINGER_EDGE_SOURCE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT, group_vtx, int_hm_seq_get(neighbors, STINGER_EDGE_DEST));
	  }
	}
	string_append_cstr(&edges, edge_str);
      } else if(q_top < vtxlimit && !found[STINGER_EDGE_DEST] && labels[STINGER_EDGE_SOURCE] == labels[STINGER_EDGE_DEST]) {
	found[STINGER_EDGE_DEST] = 1;
	queue[q_top++] = STINGER_EDGE_DEST;
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  string_append_cstr(&vertices, "\n], ");
  string_append_cstr(&edges, "\n]\n} ");

  string_t * group_str = string_new_from_cstr("{\n\t");
  string_append_string(group_str, &vertices);
  string_append_string(group_str, &edges);

  int_hm_seq_free(neighbors);
  free(found);
  free(queue);
  fclose(vtx_file);
  string_free_internal(&vertices);
  string_free_internal(&edges);
  free (vtx_str);
  return group_str;
}
