#include "pagerank_test.h"

#define restrict

class PagerankPrincetonTest : public ::testing::Test {
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

    // A->B
    // A<->C
    // B->C
    // D->C
    stinger_insert_edge(S,0,0,1,1,1);
    stinger_insert_edge_pair(S,0,0,2,1,1);
    stinger_insert_edge(S,0,1,2,1,1);
    stinger_insert_edge(S,0,3,2,1,1);
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
  double * tmp_pr;
  double * pr;
};

TEST_F(PagerankPrincetonTest, DirectedPagerank) {
  tmp_pr = (double *)xcalloc(stinger_max_active_vertex(S)+1, sizeof(double));
  pr = (double *)xcalloc(stinger_max_active_vertex(S)+1, sizeof(double));
  
  page_rank(S, stinger_max_active_vertex(S)+1, pr, tmp_pr, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, 100);

  double expected_pr[4] = {
    1.49 / 4.0,
    0.78 / 4.0,
    1.58 / 4.0,
    0.15 / 4.0
  };

  double tol = 0.001;

  for (uint64_t v=0; v < stinger_max_active_vertex(S) + 1; v++) {
    EXPECT_NEAR(expected_pr[v],pr[v],tol);
  }

  xfree(tmp_pr);
  xfree(pr);
}

TEST_F(PagerankPrincetonTest, PagerankSubset) {
  for (int i=10; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, 0, i, j, 1, timestamp);
    }
  }

  int64_t nv = stinger_max_active_vertex(S)+1;

  uint8_t * vtx_subset = (uint8_t *)xcalloc(nv,sizeof(uint8_t));
  tmp_pr = (double *)xcalloc(nv, sizeof(double));
  pr = (double *)xcalloc(nv, sizeof(double));

  for (int64_t i = 0; i < 4; i++) {
    vtx_subset[i] = 1;
  }

  page_rank_subset(S, nv, vtx_subset, 4, pr, tmp_pr, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, 100);

  xfree(vtx_subset);

  double expected_pr[4] = {
    1.49 / 4.0,
    0.78 / 4.0,
    1.58 / 4.0,
    0.15 / 4.0
  };

  double tol = 0.001;

  for (uint64_t v=0; v < stinger_max_active_vertex(S) + 1; v++) {
    if (v < 4) {
      EXPECT_NEAR(expected_pr[v],pr[v],tol);
    } else {
      EXPECT_DOUBLE_EQ(0.0,pr[v]);
    }
  }

  xfree(tmp_pr);
  xfree(pr);
}

TEST_F(PagerankPrincetonTest, PagerankDirectedType) {
  for (int i=10; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, 1, i, j, 1, timestamp);
    }
  }

  int64_t nv = stinger_max_active_vertex(S)+1;

  tmp_pr = (double *)xcalloc(nv, sizeof(double));
  pr = (double *)xcalloc(nv, sizeof(double));

  for(int64_t v = 0; v < nv; v++) {
    pr[v] = 1 / ((double)nv);
  }

  page_rank_type(S, nv, pr, tmp_pr, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, 100, 0);

  double expected_pr[4] = {
    1.49,
    0.78,
    1.58,
    0.15
  };

  double tol = 0.01;
  double ratio = expected_pr[0] / pr[0];

  double leftover = 1.0;

  for (uint64_t v=0; v < stinger_max_active_vertex(S) + 1; v++) {
    if (v < 4) {
      EXPECT_NEAR(expected_pr[v],pr[v]*ratio,tol);
      leftover -= pr[v];
    } else {
      EXPECT_NEAR(leftover / 96.0,pr[v],.0000001);
    }
  }

  xfree(tmp_pr);
  xfree(pr);
}
