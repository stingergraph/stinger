#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


static int
compare (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

int64_t 
JSON_RPC_adamic_adar::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool strings;
  bool include_neighbors;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"include_neighbors", TYPE_BOOL, &include_neighbors, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  rapidjson::Value src, src_str;
  rapidjson::Value name, vtx_phys, value;
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);


  int64_t source_deg = stinger_outdegree (S, source);

  /* this part of the response needs to be set no matter what */
  src.SetInt64(source);
  result.AddMember("source", src, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if(-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    src_str.SetString(physID, len, allocator);
    result.AddMember("source_str", src_str, allocator);
  }

  /* vertex has no edges -- this is easy */
  if (source_deg == 0) {
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);
    return 0;
  }

  int64_t * source_adj = (int64_t *) xmalloc (source_deg * sizeof(int64_t));

  /* extract and sort the neighborhood of SOURCE */
  size_t res;
  stinger_gather_successors (S, source, &res, source_adj, NULL, NULL, NULL, NULL, source_deg);
  qsort (source_adj, source_deg, sizeof(int64_t), compare);

  /* create a marks array to unique-ify the vertex list */
  uint64_t max_nv = S->max_nv;
  int64_t * marks = (int64_t *) xmalloc (max_nv * sizeof(int64_t));

  for (int64_t i = 0; i < max_nv; i++) {
    marks[i] = 0;
  }
  marks[source] = 1;

  /* calculate the maximum number of vertices in the two-hop neighborhood */
  int64_t two_hop_size = 0;
  for (int64_t i = 0; i < source_deg; i++) {
    int64_t v = source_adj[i];
    int64_t v_deg = stinger_outdegree(S, v);
    marks[v] = 1;
    two_hop_size += v_deg;
  }

  /* put everyone in the two hop neighborhood in a list (only once) */
  int64_t head = 0;
  int64_t * two_hop_neighborhood = (int64_t *) xmalloc (two_hop_size * sizeof(int64_t));

  for (int64_t i = 0; i < source_deg; i++) {
    int64_t v = source_adj[i];
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      if (stinger_int64_fetch_add(&marks[STINGER_EDGE_DEST], 1) == 0) {
	int64_t loc = stinger_int64_fetch_add(&head, 1);
	two_hop_neighborhood[loc] = STINGER_EDGE_DEST;
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  /* sanity check */
  if (head > two_hop_size) {
    LOG_W ("over ran the buffer");
  }
  two_hop_size = head;  // limit the computation later

  /* calculate the Adamic-Adar score for each vertex in level 1 */
  if (include_neighbors) {
    for (int64_t k = 0; k < source_deg; k++) {
      double adamic_adar_score = 0.0;
      int64_t vtx = source_adj[k];
      int64_t v_deg = stinger_outdegree(S, vtx);

      int64_t * target_adj = (int64_t *) xmalloc (v_deg * sizeof(int64_t));
      size_t res;
      stinger_gather_successors (S, vtx, &res, target_adj, NULL, NULL, NULL, NULL, v_deg);
      qsort (target_adj, v_deg, sizeof(int64_t), compare);

      int64_t i = 0, j = 0;
      while (i < source_deg && j < v_deg)
      {
	if (source_adj[i] < target_adj[j]) {
	  i++;
	}
	else if (target_adj[j] < source_adj[i]) {
	  j++;
	}
	else /* if source_adj[i] == target_adj[j] */
	{
	  int64_t neighbor = source_adj[i];
	  int64_t my_deg = stinger_outdegree(S, neighbor);
	  double score = 1.0 / log ((double) my_deg);
	  adamic_adar_score += score;
	  i++;
	  j++;
	}
      }

      free (target_adj);

      /* adamic_adar_score done for (source, vtx) */
      name.SetInt64(vtx);
      vtx_id.PushBack(name, allocator);
      if (strings) {
	char * physID;
	uint64_t len;
	if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
	  physID = (char *) "";
	  len = 0;
	}
	vtx_phys.SetString(physID, len, allocator);
	vtx_str.PushBack(vtx_phys, allocator);
      }
      value.SetDouble(adamic_adar_score);
      vtx_val.PushBack(value, allocator);

    }
  }

  /* calculate the Adamic-Adar score for each vertex in level 2 */
  for (int64_t k = 0; k < two_hop_size; k++) {
    double adamic_adar_score = 0.0;
    int64_t vtx = two_hop_neighborhood[k];
    int64_t v_deg = stinger_outdegree(S, vtx);

    int64_t * target_adj = (int64_t *) xmalloc (v_deg * sizeof(int64_t));
    size_t res;
    stinger_gather_successors (S, vtx, &res, target_adj, NULL, NULL, NULL, NULL, v_deg);
    qsort (target_adj, v_deg, sizeof(int64_t), compare);

    int64_t i = 0, j = 0;
    while (i < source_deg && j < v_deg)
    {
      if (source_adj[i] < target_adj[j]) {
	i++;
      }
      else if (target_adj[j] < source_adj[i]) {
	j++;
      }
      else /* if source_adj[i] == target_adj[j] */
      {
	int64_t neighbor = source_adj[i];
	int64_t my_deg = stinger_outdegree(S, neighbor);
	double score = 1.0 / log ((double) my_deg);
	adamic_adar_score += score;
	i++;
	j++;
      }
    }

    free (target_adj);

    /* adamic_adar_score done for (source, vtx) */
    name.SetInt64(vtx);
    vtx_id.PushBack(name, allocator);
    if (strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
	physID = (char *) "";
	len = 0;
      }
      vtx_phys.SetString(physID, len, allocator);
      vtx_str.PushBack(vtx_phys, allocator);
    }
    value.SetDouble(adamic_adar_score);
    vtx_val.PushBack(value, allocator);
  }

  /* done with computation, free memory */
  free (source_adj);
  free (marks);
  free (two_hop_neighborhood);
 
  /* configure the response */
  result.AddMember("vertex_id", vtx_id, allocator);
  if (strings)
    result.AddMember("vertex_str", vtx_str, allocator);
  result.AddMember("value", vtx_val, allocator);

  return 0;
}
