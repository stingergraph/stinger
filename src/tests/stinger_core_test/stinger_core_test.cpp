#include "stinger_core_test.h"
extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_traversal.h"
  #include "stinger_core/stinger_atomics.h"
}

class StingerCoreTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
    stinger_config->nv = 1<<13;
    stinger_config->nebs = 1<<16;
    stinger_config->netypes = 2;
    stinger_config->nvtypes = 2;
    stinger_config->memory_size = 1<<30;
    S = stinger_new_full(stinger_config);
    xfree(stinger_config);
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
};

class StingerSharedCoreTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    char * graph_name = (char*)xcalloc(20,sizeof(char));
    sprintf(graph_name, "/stinger-default");
    stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
    stinger_config->nv = 1<<13;
    stinger_config->nebs = 1<<16;
    stinger_config->netypes = 2;
    stinger_config->nvtypes = 2;
    stinger_config->memory_size = 1<<26;
    S = stinger_shared_new_full((char**)&graph_name,stinger_config);
    xfree(stinger_config);
  }

  virtual void TearDown() {
    size_t graph_sz = S->length + sizeof(struct stinger);
    stinger_shared_free(S,graph_name,graph_sz);
    xfree(graph_name);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
  char * graph_name;
};

TEST(StingerCoreCreationTest, AllocateStinger) {
  struct stinger_config_t * stinger_config;
  struct stinger * S;
  stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  stinger_config->nv = 1<<13;
  stinger_config->nebs = 1<<16;
  stinger_config->netypes = 2;
  stinger_config->nvtypes = 2;
  stinger_config->memory_size = 1<<30;
  S = stinger_new_full(stinger_config);
  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
  xfree(stinger_config);
  stinger_free_all(S);
}

TEST(StingerCoreCreationTest, AllocateSharedStinger) {
  struct stinger_config_t * stinger_config;
  struct stinger * S;
  char * graph_name = (char*)xcalloc(20,sizeof(char));
  sprintf(graph_name, "/stinger-default");
  stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  stinger_config->nv = 1<<13;
  stinger_config->nebs = 1<<16;
  stinger_config->netypes = 2;
  stinger_config->nvtypes = 2;
  stinger_config->memory_size = 1<<26;
  S = stinger_shared_new_full(&graph_name,stinger_config);
  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
  xfree(stinger_config);
  size_t graph_sz = S->length + sizeof(struct stinger);
  stinger_shared_free(S,graph_name,graph_sz);
  xfree(graph_name);
}

TEST(StingerCoreCreationTest, AllocateLargeStinger) {
  struct stinger_config_t * stinger_config;
  struct stinger * S;
  stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  stinger_config->nv = 0;
  stinger_config->nebs = 0;
  stinger_config->netypes = 0;
  stinger_config->nvtypes = 0;
  stinger_config->memory_size = 1<<30;
  S = stinger_new_full(stinger_config);
  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
  xfree(stinger_config);
  stinger_free_all(S);
}

TEST(StingerCoreCreationTest, AllocateLargeSharedStinger) {
  struct stinger_config_t * stinger_config;
  struct stinger * S;
  char * graph_name = (char*)xcalloc(20,sizeof(char));
  sprintf(graph_name, "/stinger-default");
  stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  
  stinger_config->nv = 0;
  stinger_config->nebs = 0;
  stinger_config->netypes = 0;
  stinger_config->nvtypes = 0;

  stinger_config->memory_size = 1<<26;
  S = stinger_shared_new_full(&graph_name,stinger_config);
  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
  xfree(stinger_config);
  size_t graph_sz = S->length + sizeof(struct stinger);
  stinger_shared_free(S,graph_name,graph_sz);
  xfree(graph_name);
}

TEST_F(StingerCoreTest, stinger_vtype_get_set) {
  vtype_t vtype;
  const vtype_t DEFAULT_TYPE = 0;
  const vtype_t INVALID_VERTEX = -1;

  vtype = stinger_vtype_get(S,0);
  EXPECT_EQ(vtype,DEFAULT_TYPE);

  vtype = stinger_vtype_get(S,S->max_nv-1);
  EXPECT_EQ(vtype,DEFAULT_TYPE);

  vtype = stinger_vtype_get(S,S->max_nv);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  vtype = stinger_vtype_get(S,S->max_nv+1);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  vtype = stinger_vtype_get(S,-1);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  vtype = stinger_vtype_set(S,0,1);
  EXPECT_EQ(vtype,1);

  vtype = stinger_vtype_set(S,S->max_nv-1,1);
  EXPECT_EQ(vtype,1);

  vtype = stinger_vtype_set(S,S->max_nv,1);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  vtype = stinger_vtype_set(S,S->max_nv+1,1);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  vtype = stinger_vtype_set(S,-1,1);
  EXPECT_EQ(vtype,INVALID_VERTEX);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, stinger_weight_manipulation) {
  vweight_t vweight;
  const vweight_t DEFAULT_WEIGHT = 0;
  const vweight_t INVALID_VERTEX = -1;

  vweight = stinger_vweight_get(S,0);
  EXPECT_EQ(vweight,DEFAULT_WEIGHT);

  vweight = stinger_vweight_get(S,S->max_nv-1);
  EXPECT_EQ(vweight,DEFAULT_WEIGHT);

  vweight = stinger_vweight_get(S,S->max_nv);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  vweight = stinger_vweight_get(S,S->max_nv+1);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  vweight = stinger_vweight_get(S,-1);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  vweight = stinger_vweight_set(S,0,1);
  EXPECT_EQ(vweight,1);

  vweight = stinger_vweight_set(S,S->max_nv-1,1);
  EXPECT_EQ(vweight,1);

  vweight = stinger_vweight_set(S,S->max_nv,1);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  vweight = stinger_vweight_set(S,S->max_nv+1,1);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  vweight = stinger_vweight_set(S,-1,1);
  EXPECT_EQ(vweight,INVALID_VERTEX);

  /* Test Serial Increment */
  vweight = stinger_vweight_increment(S, 1, 100);
  vweight = stinger_vweight_get(S,1);
  EXPECT_EQ(vweight,100);

  for (int i=0; i < 1000; i++) {
    vweight = stinger_vweight_increment(S, 2, 1);
  }
  vweight = stinger_vweight_get(S,2);
  EXPECT_EQ(vweight,1000);

  /* Test Parallel Increment */
  #pragma omp parallel for
  for (int i=0; i < 1000; i++) {
    vweight = stinger_vweight_increment_atomic(S, 3, 1);
  }
  vweight = stinger_vweight_get(S,3);
  EXPECT_EQ(vweight,1000);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, stinger_vertex_adjacency_test) {
/*
adjacency_t
stinger_adjacency_get(const stinger_t * S, vindex_t v)
*/
  struct stinger_eb * eb;
  MAP_STING(S);

  eb = ebpool->ebpool + stinger_adjacency_get(S, 1);

  EXPECT_EQ(eb,ebpool->ebpool);

  stinger_insert_edge(S, 0, 1, 2, 1, 1);

  eb = ebpool->ebpool + stinger_adjacency_get(S, 1);

  EXPECT_EQ(eb->vertexID,1);
  EXPECT_EQ(eb->etype,0);
  EXPECT_EQ(eb->high,1);
  EXPECT_EQ(eb->smallStamp,1);
  EXPECT_EQ(eb->largeStamp,1);
  EXPECT_EQ(ebpool->ebpool + eb->next,ebpool->ebpool);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, stinger_check_setup) {
  vindex_t max_nv = stinger_max_nv(S);
  EXPECT_EQ(max_nv, 1<<13);

  int64_t max_netypes = stinger_max_num_etypes(S);
  EXPECT_EQ(max_netypes, 2);
}

TEST_F(StingerCoreTest, stinger_edge_insertion) {
  int64_t total_edges = 0;
  // Insert undirected edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
      EXPECT_EQ(ret,3);
      total_edges += 2;
    }
  }

  // Re-insert the same edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
      EXPECT_EQ(ret,0);
    }
  }

  // Insert directed edges on type 1
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge(S, 1, i, j, 1, timestamp);
      EXPECT_EQ(ret,1);
      total_edges += 1;
    }
  }

  // Re-insert directed edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge(S, 1, i, j, 1, timestamp);
      EXPECT_EQ(ret,0);
    }
  }

  // Increment undirected edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_incr_edge_pair(S, 0, i, j, 1, timestamp);
      EXPECT_EQ(ret,0);
    }
  }

  // Increment directed edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_incr_edge(S, 1, i, j, 1, timestamp);
      EXPECT_EQ(ret,0);
    }
  }

  // Increment new undirected edges
  for (int i=100; i < 200; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 200; j++) {
      ret = stinger_incr_edge_pair(S, 0, i, j, 1, timestamp);
      EXPECT_EQ(ret,3);
      total_edges += 2;
    }
  }

  // Increment new directed edges
  for (int i=100; i < 200; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 200; j++) {
      ret = stinger_incr_edge(S, 1, i, j, 1, timestamp);
      EXPECT_EQ(ret,1);
      total_edges += 1;
    }
  }

  // Increment partially new undirected edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_incr_edge_pair(S, 1, i, j, 1, timestamp);
      EXPECT_EQ(ret,2);
      total_edges += 1;
    }
  }

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    int64_t expected_timestamp = std::min(STINGER_EDGE_SOURCE,STINGER_EDGE_DEST) + 1;
    total_edges--;
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  EXPECT_EQ(total_edges,0);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, stinger_delete_edges) {
  int64_t ret;

  /* Part 1 - Insert edge pair, remove edge pair */

  stinger_insert_edge_pair(S, 0, 300, 301, 1, 1);

  int64_t outDegree, inDegree, degree;

  outDegree = stinger_outdegree_get(S,300);
  inDegree = stinger_indegree_get(S,300);
  degree = stinger_degree_get(S,300);

  EXPECT_EQ(outDegree,1);
  EXPECT_EQ(inDegree,1);
  EXPECT_EQ(degree,1);

  outDegree = stinger_outdegree_get(S,301);
  inDegree = stinger_indegree_get(S,301);
  degree = stinger_degree_get(S,301);

  EXPECT_EQ(outDegree,1);
  EXPECT_EQ(inDegree,1);
  EXPECT_EQ(degree,1);

  ret = stinger_remove_edge_pair(S, 0, 300, 301);

  EXPECT_EQ(ret,3);

  outDegree = stinger_outdegree_get(S,300);
  inDegree = stinger_indegree_get(S,300);
  degree = stinger_degree_get(S,300);

  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,0);

  outDegree = stinger_outdegree_get(S,301);
  inDegree = stinger_indegree_get(S,301);
  degree = stinger_degree_get(S,301);

  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,0);

  int64_t total_edges = 0;

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    total_edges++;
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

  EXPECT_EQ(total_edges,0);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  /* Part 2 - Remove edge pair from empty graph */

  ret = stinger_remove_edge_pair(S, 0, 300, 301);

  EXPECT_EQ(ret,-1);

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  /* Part 3 - Remove single edge */

  stinger_insert_edge_pair(S, 0, 400, 401, 1, 1);

  ret = stinger_remove_edge(S, 0, 400, 401);

  EXPECT_EQ(ret,1);

  outDegree = stinger_outdegree_get(S,400);
  inDegree = stinger_indegree_get(S,400);
  degree = stinger_degree_get(S,400);

  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,1);
  EXPECT_EQ(degree,1);

  outDegree = stinger_outdegree_get(S,401);
  inDegree = stinger_indegree_get(S,401);
  degree = stinger_degree_get(S,400);

  EXPECT_EQ(outDegree,1);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,1);

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    total_edges++;
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

  EXPECT_EQ(total_edges,1);

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  // Part 4 - Test Parallel Deletion

  // Insert undirected edges
  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  // Insert directed edges on type 1
  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge(S, 1, i, j, 1, timestamp);
    }
  }

  for (int i=0; i < 100; i++) {
    OMP("omp parallel for")
    for (int j=i+1; j < 100; j++) {
      ret = stinger_remove_edge_pair(S, 0, i, j);
    }
  }

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  for (int i=0; i < 100; i++) {
    outDegree = stinger_outdegree_get(S,i);
    inDegree = stinger_indegree_get(S,i);
    degree = stinger_degree_get(S,i);
    EXPECT_EQ(outDegree,99-i);
    EXPECT_EQ(inDegree,i);
    EXPECT_EQ(degree,99);
    outDegree = stinger_typed_outdegree(S,i,0);
    degree = stinger_typed_degree(S,i,0);
    EXPECT_EQ(outDegree,0);
    EXPECT_EQ(degree,0);
    outDegree = stinger_typed_outdegree(S,i,1);
    degree = stinger_typed_degree(S,i,1);
    EXPECT_EQ(outDegree,99-i);
    EXPECT_EQ(degree,99);
  }

  for (int i=0; i < 100; i++) {
    OMP("omp parallel for")
    for (int j=i+1; j < 100; j++) {
      ret = stinger_remove_edge(S, 1, i, j);
    }
  }

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  for (int i=0; i < 100; i++) {
    outDegree = stinger_outdegree_get(S,i);
    inDegree = stinger_indegree_get(S,i);
    degree = stinger_degree_get(S,i);
    EXPECT_EQ(outDegree,0);
    EXPECT_EQ(inDegree,0);
    EXPECT_EQ(degree,0);
  }
}

TEST_F(StingerCoreTest, racing_deletions) {
  int64_t ret;
  // Insert undirected edges
  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  for (int i=0; i < 100; i++) {
    for (int j=i+1; j < 100; j++) {
      OMP("omp parallel") {
        OMP ("omp sections") {
          OMP("omp section") {
            ret = stinger_remove_edge(S, 0, i, j);
          }
          OMP("omp section") {
            ret = stinger_remove_edge(S, 0, j, i);
          }
        }
      }
    }
  }

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, stinger_remove_vertex) {
  int64_t consistency;
  // Insert undirected edges
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    int64_t ret;
    for (int j=i+1; j < 100; j++) {
      ret = stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  // Insert some directed out edges from vertex 1
  for (int i=101; i < 500; i+=4) {
    int64_t timestamp = 2;
    int64_t ret;    
    ret = stinger_insert_edge(S, 0, 1, i, 1, timestamp);
  }

  // Insert some directed in edges to vertex 2
  for (int i=102; i < 500; i+=4) {
    int64_t timestamp = 2;
    int64_t ret;    
    ret = stinger_insert_edge(S, 0, i, 2, 1, timestamp);
  }

  // PART 1: remove a vertex with all undirected edges
  int64_t outDegree, inDegree, degree;
  outDegree = stinger_outdegree_get(S,3);
  inDegree = stinger_indegree_get(S,3);
  degree = stinger_degree_get(S,3);
  EXPECT_GT(outDegree,0);
  EXPECT_GT(inDegree,0);
  EXPECT_GT(degree,0);
  stinger_remove_vertex (S, 3);
  outDegree = stinger_outdegree_get(S,3);
  inDegree = stinger_indegree_get(S,3);
  degree = stinger_degree_get(S,3);
  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,0);

  bool has_edge_list = false;
  bool in_edge_list = false;

  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,3) {
    fprintf(stderr,"%ld %ld %ld\n",STINGER_EDGE_SOURCE,STINGER_EDGE_DEST,STINGER_EDGE_DIRECTION);
    has_edge_list = true;
  } STINGER_FORALL_EDGES_OF_VTX_END();
  EXPECT_FALSE(has_edge_list) << "Vertex should have no incident edges";

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    if (STINGER_EDGE_SOURCE == 3 || STINGER_EDGE_DEST == 3) {
      in_edge_list = true;
    }
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  EXPECT_FALSE(in_edge_list) << "Vertex should have no incident edges";

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);

  // PART 2: remove a vertex with some directed out edges
  outDegree = stinger_outdegree_get(S,1);
  inDegree = stinger_indegree_get(S,1);
  degree = stinger_degree_get(S,1);
  EXPECT_GT(outDegree,0);
  EXPECT_GT(inDegree,0);
  EXPECT_GT(degree,0);
  stinger_remove_vertex (S, 1);
  outDegree = stinger_outdegree_get(S,1);
  inDegree = stinger_indegree_get(S,1);
  degree = stinger_degree_get(S,1);
  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,0);

  has_edge_list = false;
  in_edge_list = false;
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,1) {
    has_edge_list = true;
  } STINGER_FORALL_EDGES_OF_VTX_END();
  EXPECT_FALSE(has_edge_list) << "Vertex should have no incident edges";

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    if (STINGER_EDGE_SOURCE == 1 || STINGER_EDGE_DEST == 1) {
      in_edge_list = true;
    }
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  EXPECT_FALSE(in_edge_list) << "Vertex should have no incident edges";

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0); 

  // PART 2: remove a vertex with some directed in edges
  outDegree = stinger_outdegree_get(S,2);
  inDegree = stinger_indegree_get(S,2);
  degree = stinger_degree_get(S,2);
  EXPECT_GT(outDegree,0);
  EXPECT_GT(inDegree,0);
  EXPECT_GT(degree,0);
  stinger_remove_vertex (S, 2);
  outDegree = stinger_outdegree_get(S,2);
  inDegree = stinger_indegree_get(S,2);
  degree = stinger_degree_get(S,2);
  EXPECT_EQ(outDegree,0);
  EXPECT_EQ(inDegree,0);
  EXPECT_EQ(degree,0);

  has_edge_list = false;
  in_edge_list = false;
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,2) {
    has_edge_list = true;
  } STINGER_FORALL_EDGES_OF_VTX_END(); 
  EXPECT_FALSE(has_edge_list) << "Vertex should have no incident edges";

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    if (STINGER_EDGE_SOURCE == 2 || STINGER_EDGE_DEST == 2) {
      in_edge_list = true;
    }
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  EXPECT_FALSE(in_edge_list) << "Vertex should have no incident edges";

  consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);  
}

TEST_F(StingerCoreTest, stinger_save_load_file) {
  // First Undirected
  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+2;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  int64_t ret;

  ret = stinger_save_to_file(S, stinger_max_active_vertex(S)+1, "./undirected.stinger");

  EXPECT_EQ(ret,0);

  // Make a new STINGER
  stinger_free_all(S);
  stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  stinger_config->nv = 1<<13;
  stinger_config->nebs = 1<<16;
  stinger_config->netypes = 2;
  stinger_config->nvtypes = 2;
  stinger_config->memory_size = 1<<30;
  S = stinger_new_full(stinger_config);
  xfree(stinger_config);

  uint64_t max_nv = 0;

  ret = stinger_open_from_file("./undirected.stinger",S,&max_nv);

  EXPECT_EQ(ret,0);
  EXPECT_EQ(max_nv,100);

  int64_t total_edges = (100*99);

  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    int64_t expected_timestamp = std::min(STINGER_EDGE_SOURCE,STINGER_EDGE_DEST) + 1;
    EXPECT_EQ(expected_timestamp+1, STINGER_EDGE_TIME_RECENT);
    EXPECT_EQ(expected_timestamp, STINGER_EDGE_TIME_FIRST);
    total_edges--;
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  EXPECT_EQ(total_edges,0);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, edge_metadata) {
  int64_t ret;
  
  stinger_insert_edge_pair(S, 0, 1, 2, 1, 100); // Edge 1->2 and 2->1

  stinger_insert_edge(S, 0, 3, 4, 2, 200); // Edge 3->4
  stinger_insert_edge(S, 0, 4, 3, 4, 300); // Edge 4->3

  stinger_insert_edge(S, 1, 3, 4, 4, 400); // Edge 3->4 Type 1
  stinger_insert_edge(S, 1, 4, 3, 2, 500); // Edge 4->3 Type 1

  // Check edge weights of all directions
  ret = stinger_edgeweight(S, 1, 2, 0);
  EXPECT_EQ(ret, 1);
  ret = stinger_edgeweight(S, 2, 1, 0);
  EXPECT_EQ(ret, 1);
  ret = stinger_edgeweight(S, 3, 4, 0);
  EXPECT_EQ(ret, 2);
  ret = stinger_edgeweight(S, 4, 3, 0);
  EXPECT_EQ(ret, 4);
  ret = stinger_edgeweight(S, 3, 4, 1);
  EXPECT_EQ(ret, 4);
  ret = stinger_edgeweight(S, 4, 3, 1);
  EXPECT_EQ(ret, 2);

  // Set an edgeweight and check
  stinger_set_edgeweight(S, 1, 2, 0, 5);
  ret = stinger_edgeweight(S, 1, 2, 0);
  EXPECT_EQ(ret, 5);
  ret = stinger_edgeweight(S, 2, 1, 0);
  EXPECT_EQ(ret, 1);

  // Set edge weight on type 1 and check it doesn't
  // affect type 0
  stinger_set_edgeweight(S, 3, 4, 1, 6);
  ret = stinger_edgeweight(S, 3, 4, 1);
  EXPECT_EQ(ret, 6);
  ret = stinger_edgeweight(S, 3, 4, 0);
  EXPECT_EQ(ret, 2);

  // Timestamp tests

  // Check initial timestamps are set
  ret = stinger_edge_timestamp_first(S, 1, 2, 0);
  EXPECT_EQ(ret, 100);
  ret = stinger_edge_timestamp_first(S, 2, 1, 0);
  EXPECT_EQ(ret, 100);
  ret = stinger_edge_timestamp_first(S, 3, 4, 0);
  EXPECT_EQ(ret, 200);
  ret = stinger_edge_timestamp_first(S, 4, 3, 0);
  EXPECT_EQ(ret, 300);
  ret = stinger_edge_timestamp_first(S, 3, 4, 1);
  EXPECT_EQ(ret, 400);
  ret = stinger_edge_timestamp_first(S, 4, 3, 1);
  EXPECT_EQ(ret, 500);

  ret = stinger_edge_timestamp_recent(S, 1, 2, 0);
  EXPECT_EQ(ret, 100);
  ret = stinger_edge_timestamp_recent(S, 2, 1, 0);
  EXPECT_EQ(ret, 100);
  ret = stinger_edge_timestamp_recent(S, 3, 4, 0);
  EXPECT_EQ(ret, 200);
  ret = stinger_edge_timestamp_recent(S, 4, 3, 0);
  EXPECT_EQ(ret, 300);
  ret = stinger_edge_timestamp_recent(S, 3, 4, 1);
  EXPECT_EQ(ret, 400);
  ret = stinger_edge_timestamp_recent(S, 4, 3, 1);
  EXPECT_EQ(ret, 500);

  // Update recent timestamps
  stinger_edge_touch(S, 1, 2, 0, 800);
  stinger_edge_touch(S, 3, 4, 1, 800);

  ret = stinger_edge_timestamp_first(S, 1, 2, 0);
  EXPECT_EQ(ret, 100);
  ret = stinger_edge_timestamp_recent(S, 1, 2, 0);
  EXPECT_EQ(ret, 800);
  ret = stinger_edge_timestamp_first(S, 3, 4, 1);
  EXPECT_EQ(ret, 400);
  ret = stinger_edge_timestamp_recent(S, 3, 4, 1);
  EXPECT_EQ(ret, 800);

  // Test non-existent edges
  ret = stinger_edgeweight(S, 8, 9, 0);
  EXPECT_EQ(ret, 0);

  ret = stinger_set_edgeweight(S, 8, 9, 0, 10);
  EXPECT_EQ(ret, 0);

  ret = stinger_edge_timestamp_first(S, 8, 9, 0);
  EXPECT_EQ(ret, -1);

  ret = stinger_edge_timestamp_recent(S, 8, 9, 0);
  EXPECT_EQ(ret, -1);

  ret = stinger_edge_touch(S, 8, 9, 0, 800);
  EXPECT_EQ(ret, 0);

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, internal_objects) {
  stinger_vertices_t * v = stinger_vertices_get(S);
  stinger_physmap_t * p = stinger_physmap_get(S);
  stinger_names_t * vt = stinger_vtype_names_get(S);
  stinger_names_t * et = stinger_etype_names_get(S);

  EXPECT_NE(v,(void*)NULL);
  EXPECT_NE(p,(void*)NULL);
  EXPECT_NE(vt,(void*)NULL);
  EXPECT_NE(et,(void*)NULL);
}

TEST_F(StingerCoreTest, active_vertices) {
  uint64_t max_active; // Max ID of active vertex (has incident edges)
  uint64_t num_active; // Number of vertices with incident edges

  max_active = stinger_max_active_vertex(S);
  num_active = stinger_num_active_vertices(S);

  EXPECT_EQ(max_active,0);
  EXPECT_EQ(num_active,0);

  stinger_insert_edge(S, 0, 0, 1, 1, 100);

  max_active = stinger_max_active_vertex(S);
  num_active = stinger_num_active_vertices(S);

  EXPECT_EQ(max_active,1);
  EXPECT_EQ(num_active,2);

  stinger_insert_edge(S, 0, 1, 100, 1, 100);

  max_active = stinger_max_active_vertex(S);
  num_active = stinger_num_active_vertices(S);

  EXPECT_EQ(max_active,100);
  EXPECT_EQ(num_active,3);

  stinger_remove_edge(S, 0, 0, 1);

  max_active = stinger_max_active_vertex(S);
  num_active = stinger_num_active_vertices(S);

  EXPECT_EQ(max_active,100);
  EXPECT_EQ(num_active,2);

  stinger_remove_vertex (S, 1);

  max_active = stinger_max_active_vertex(S);
  num_active = stinger_num_active_vertices(S);

  EXPECT_EQ(max_active,0);
  EXPECT_EQ(num_active,0);  

  int64_t consistency = stinger_consistency_check(S,S->max_nv);
  EXPECT_EQ(consistency,0);
}

TEST_F(StingerCoreTest, gather_successors) {
  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, (j%2)?1:0 , i, j, 1, timestamp);
    }
  }

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+2;
    for (int j=i+101; j < 200; j++) {
      stinger_insert_edge(S, 0, j, i, 1, timestamp);
    }
  }

  // PART 1-1: Try a buffer that is big enough to hold all successors
  int64_t * out_vtx = (int64_t *)xcalloc(200,sizeof(int64_t));
  int64_t * out_weight = (int64_t *)xcalloc(200,sizeof(int64_t));
  int64_t * out_timefirst = (int64_t *)xcalloc(200,sizeof(int64_t));
  int64_t * out_timerecent = (int64_t *)xcalloc(200,sizeof(int64_t));
  int64_t * out_type = (int64_t *)xcalloc(200,sizeof(int64_t));

  size_t outlen;
  int64_t ret;

  stinger_gather_successors(S, 0, &outlen, out_vtx, out_weight, out_timefirst, out_timerecent, out_type, 200);
  EXPECT_EQ(outlen, 99);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_LT(out_vtx[i],101);  // The edges to vertices >100 are all in edges
    EXPECT_EQ(out_weight[i],1);
    EXPECT_EQ(out_timefirst[i],1);
    EXPECT_EQ(out_timerecent[i],1);
    EXPECT_EQ(out_type[i],(out_vtx[i]%2)?1:0); // Make sure both types of edges are collected
  }

  xfree(out_vtx);
  xfree(out_weight);
  xfree(out_timefirst);
  xfree(out_timerecent);
  xfree(out_type);

  // PART 1-2: Gather typed successors
  out_vtx = (int64_t *)xcalloc(200,sizeof(int64_t));

  stinger_gather_typed_successors(S, 0, 0, &outlen, out_vtx, 200);
  EXPECT_EQ(outlen, 49);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_EQ((out_vtx[i]%2),0);
    EXPECT_LT(out_vtx[i],101);  
  }

  ret = stinger_has_typed_successor(S, 1, 0, 1);

  EXPECT_EQ(ret,1);

  ret = stinger_has_typed_successor(S, 0, 101, 1);

  EXPECT_EQ(ret,0);

  xfree(out_vtx);

  // PART 2-1: Try a buffer that is too small for all successors
  out_vtx = (int64_t *)xcalloc(50,sizeof(int64_t));
  out_weight = (int64_t *)xcalloc(50,sizeof(int64_t));
  out_timefirst = (int64_t *)xcalloc(50,sizeof(int64_t));
  out_timerecent = (int64_t *)xcalloc(50,sizeof(int64_t));
  out_type = (int64_t *)xcalloc(50,sizeof(int64_t));

  stinger_gather_successors(S, 0, &outlen, out_vtx, out_weight, out_timefirst, out_timerecent, out_type, 50);
  EXPECT_EQ(outlen, 50);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_LT(out_vtx[i],101);  // The edges to vertices >100 are all in edges
    EXPECT_EQ(out_weight[i],1);
    EXPECT_EQ(out_timefirst[i],1);
    EXPECT_EQ(out_timerecent[i],1);
    EXPECT_EQ(out_type[i],(out_vtx[i]%2)?1:0); // Make sure both types of edges are collected
  }

  xfree(out_vtx);
  xfree(out_weight);
  xfree(out_timefirst);
  xfree(out_timerecent);
  xfree(out_type);

  // PART 2-2: Typed successors
  out_vtx = (int64_t *)xcalloc(25,sizeof(int64_t));

  stinger_gather_typed_successors(S, 0, 0, &outlen, out_vtx, 25);
  EXPECT_EQ(outlen, 25);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_EQ((out_vtx[i]%2),0);
    EXPECT_LT(out_vtx[i],101);  
  }

  xfree(out_vtx);

  // PART 3-1: Try an empty buffer
  out_vtx = NULL;
  out_weight = NULL;
  out_timefirst = NULL;
  out_timerecent = NULL;
  out_type = NULL;

  stinger_gather_successors(S, 0, &outlen, out_vtx, out_weight, out_timefirst, out_timerecent, out_type, 0);
  EXPECT_EQ(outlen, 0);

  // PART 3-2: Typed successors
  stinger_gather_typed_successors(S, 0, 0, &outlen, out_vtx, 0);
  EXPECT_EQ(outlen, 0);
}

TEST_F(StingerCoreTest, gather_predecessors) {
  void stinger_gather_typed_predecessors (const struct stinger *,
    int64_t /* type */,
    int64_t /* v */,
    size_t * /* outlen */,
    int64_t * /* out */,
    size_t /* max_outlen */ );  

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, (j%2)?1:0 , i, j, 1, timestamp);
    }
  }

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+2;
    for (int j=i+101; j < 200; j++) {
      stinger_insert_edge(S, 0, j, i, 1, timestamp);
    }
  }

  // PART 1: Try a buffer that is big enough to hold all predecessors
  int64_t * out_vtx = (int64_t *)xcalloc(200,sizeof(int64_t));
  size_t outlen;

  stinger_gather_typed_predecessors(S, 0, 0, &outlen, out_vtx, 200);
  EXPECT_EQ(outlen, 148);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_TRUE(out_vtx[i] % 2 == 0 || out_vtx[i] >= 101);
  }

  xfree(out_vtx);

  // PART 2: Try a small buffer
  out_vtx = (int64_t *)xcalloc(99,sizeof(int64_t));

  stinger_gather_typed_predecessors(S, 0, 0, &outlen, out_vtx, 99);
  EXPECT_EQ(outlen, 99);

  for (int64_t i=0; i < outlen; i++) {
    EXPECT_TRUE(out_vtx[i] % 2 == 0 || out_vtx[i] >= 101);
  }

  xfree(out_vtx);

  // PART 3: Try an empty buffer
  out_vtx = NULL;

  stinger_gather_typed_predecessors(S, 0, 0, &outlen, out_vtx, 0);
  EXPECT_EQ(outlen, 0);
  EXPECT_EQ(out_vtx, (void*)NULL);
}

TEST_F(StingerCoreTest, edge_counts) {
  int64_t edge_counts[2][200];
  int64_t extra_in_edges = 0;

  for (int i=0; i < 200; i++) {
    edge_counts[0][i] = edge_counts[1][i] = 0;
  }

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, (j%2)?1:0 , i, j, 1, timestamp);
      // A pair should add one edge on each size
      stinger_int64_fetch_add(&edge_counts[(j%2)?1:0][i],1);
      stinger_int64_fetch_add(&edge_counts[(j%2)?1:0][j],1);
    }
  }

  OMP("omp parallel for")
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+2;
    for (int j=i+101; j < 200; j++) {
      // a single edge should add an In edge and an out edge
      stinger_insert_edge(S, 0, j, i, 1, timestamp);
      stinger_int64_fetch_add(&edge_counts[0][j],1);
      stinger_int64_fetch_add(&edge_counts[0][i],1);
      stinger_int64_fetch_add(&extra_in_edges,1);
    }
  }

  int64_t total_edges = stinger_total_edges(S);
  int64_t edges_up_to = stinger_edges_up_to(S,100);
  int64_t max_edges = stinger_max_total_edges(S);

  int64_t expected_max_ebs = 0;
  int64_t expected_total_edges = 0;
  int64_t expected_edges_up_to = 0;

  for (int i=0; i < 200; i++) {
    expected_max_ebs += (int64_t)ceil((double)edge_counts[0][i] / STINGER_EDGEBLOCKSIZE);
    expected_max_ebs += (int64_t)ceil((double)edge_counts[1][i] / STINGER_EDGEBLOCKSIZE);

    expected_total_edges += edge_counts[0][i] + edge_counts[1][i];

    if (i < 100) {
      expected_edges_up_to += edge_counts[0][i] + edge_counts[1][i];
    }
  }
  expected_edges_up_to -= extra_in_edges;
  expected_total_edges -= extra_in_edges;

  EXPECT_EQ(max_edges, (expected_max_ebs+1) * STINGER_EDGEBLOCKSIZE);

  EXPECT_EQ(edges_up_to, expected_edges_up_to);

  EXPECT_EQ(total_edges, expected_total_edges);
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
