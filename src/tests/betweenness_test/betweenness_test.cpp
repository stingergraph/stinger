#include "betweenness_test.h"

#define restrict

class BetweennessTest : public ::testing::Test {
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

TEST_F(BetweennessTest, DirectedGraph) {
  stinger_insert_edge(S, 0, 0, 1, 1, 1);
  stinger_insert_edge(S, 0, 1, 2, 1, 1);
  stinger_insert_edge(S, 0, 1, 3, 1, 1);
  stinger_insert_edge(S, 0, 1, 4, 1, 1);
  stinger_insert_edge(S, 0, 2, 8, 1, 1);
  stinger_insert_edge(S, 0, 3, 5, 1, 1);
  stinger_insert_edge(S, 0, 3, 6, 1, 1);
  stinger_insert_edge(S, 0, 4, 5, 1, 1);
  stinger_insert_edge(S, 0, 5, 6, 1, 1);
  stinger_insert_edge(S, 0, 5, 7, 1, 1);
  stinger_insert_edge(S, 0, 7, 8, 1, 1);

  int64_t nv = stinger_max_active_vertex(S)+1;

  double * bc = (double *)xcalloc(nv, sizeof(double));
  int64_t * times_found = (int64_t *)xcalloc(nv, sizeof(int64_t)); 

  sample_search(S, nv, 9, bc, times_found);

  double expected_bc[9] = {
    0.0,
    7.0,
    2.0,
    4.0,
    2.0,
    7.0,
    0.0,
    3.0,
    0.0
  };

  for (int64_t v = 0; v < nv; v++) {
    EXPECT_DOUBLE_EQ(expected_bc[v],bc[v]) << "v = " << v;
  }

  xfree(bc);
  xfree(times_found);
}

TEST_F(BetweennessTest, DirectedGraphOverSample) {
  stinger_insert_edge(S, 0, 0, 1, 1, 1);
  stinger_insert_edge(S, 0, 1, 2, 1, 1);
  stinger_insert_edge(S, 0, 1, 3, 1, 1);
  stinger_insert_edge(S, 0, 1, 4, 1, 1);
  stinger_insert_edge(S, 0, 2, 8, 1, 1);
  stinger_insert_edge(S, 0, 3, 5, 1, 1);
  stinger_insert_edge(S, 0, 3, 6, 1, 1);
  stinger_insert_edge(S, 0, 4, 5, 1, 1);
  stinger_insert_edge(S, 0, 5, 6, 1, 1);
  stinger_insert_edge(S, 0, 5, 7, 1, 1);
  stinger_insert_edge(S, 0, 7, 8, 1, 1);

  int64_t nv = stinger_max_active_vertex(S)+1;

  double * bc = (double *)xcalloc(nv, sizeof(double));
  int64_t * times_found = (int64_t *)xcalloc(nv, sizeof(int64_t)); 

  sample_search(S, nv, 100, bc, times_found);

  double expected_bc[9] = {
    0.0,
    7.0,
    2.0,
    4.0,
    2.0,
    7.0,
    0.0,
    3.0,
    0.0
  };

  for (int64_t v = 0; v < nv; v++) {
    EXPECT_DOUBLE_EQ(expected_bc[v],bc[v]) << "v = " << v;
  }

  xfree(bc);
  xfree(times_found);
}

TEST_F(BetweennessTest, UndirectedGraph) {
  stinger_insert_edge_pair(S, 0, 0, 1, 1, 1);
  stinger_insert_edge_pair(S, 0, 1, 2, 1, 1);
  stinger_insert_edge_pair(S, 0, 1, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 1, 4, 1, 1);
  stinger_insert_edge_pair(S, 0, 2, 8, 1, 1);
  stinger_insert_edge_pair(S, 0, 3, 5, 1, 1);
  stinger_insert_edge_pair(S, 0, 3, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 4, 5, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 7, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 8, 1, 1);

  int64_t nv = stinger_max_active_vertex(S)+1;

  double * bc = (double *)xcalloc(nv, sizeof(double));
  int64_t * times_found = (int64_t *)xcalloc(nv, sizeof(int64_t)); 

  sample_search(S, nv, 9, bc, times_found);

  double expected_bc[9] = {
    0.0,
    24.333333333333,
    7.333333333333,
    10.0,
    4.0,
    15.666666666666,
    0.0,
    6.666666666666,
    4.0
  };

  for (int64_t v = 0; v < nv; v++) {
    EXPECT_NEAR(expected_bc[v],bc[v],0.00001) << "v = " << v;
  }

  xfree(bc);
  xfree(times_found);
}

TEST_F(BetweennessTest, Subgraph) {
  stinger_insert_edge(S, 0, 0, 1, 1, 1);
  stinger_insert_edge(S, 0, 1, 2, 1, 1);
  stinger_insert_edge(S, 0, 1, 3, 1, 1);
  stinger_insert_edge(S, 0, 1, 4, 1, 1);
  stinger_insert_edge(S, 0, 2, 8, 1, 1);
  stinger_insert_edge(S, 0, 3, 5, 1, 1);
  stinger_insert_edge(S, 0, 3, 6, 1, 1);
  stinger_insert_edge(S, 0, 4, 5, 1, 1);
  stinger_insert_edge(S, 0, 5, 6, 1, 1);
  stinger_insert_edge(S, 0, 5, 7, 1, 1);
  stinger_insert_edge(S, 0, 7, 8, 1, 1);

  int64_t nv = stinger_max_active_vertex(S)+1;

  double * bc = (double *)xcalloc(nv, sizeof(double));
  int64_t * times_found = (int64_t *)xcalloc(nv, sizeof(int64_t)); 
  uint8_t * vtx_set = (uint8_t *)xcalloc(nv,sizeof(uint8_t));

  for (int64_t i = 0; i < nv; i++) {
    if (i != 3) {
      vtx_set[i] = 1;
    }
  }

  sample_search_subgraph(S, nv, vtx_set, 9, bc, times_found);

  xfree(vtx_set);

  double expected_bc[9] = {
    0.0,
    6.0,
    2.0,
    0.0,
    6.0,
    7.0,
    0.0,
    2.0,
    0.0
  };

  for (int64_t v = 0; v < nv; v++) {
    EXPECT_DOUBLE_EQ(expected_bc[v],bc[v]) << "v = " << v;
  }

  xfree(bc);
  xfree(times_found);
}


int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

