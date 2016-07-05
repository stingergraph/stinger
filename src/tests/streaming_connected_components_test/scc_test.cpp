#include "scc_test.h"

#define restrict

class SCCTest : public ::testing::Test {
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


TEST_F(SCCTest, UndirectedGraph) {
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

  int64_t nv = stinger_max_nv(S);
  stinger_scc_internal scc_internal;
  stinger_scc_initialize_internals(S,nv,&scc_internal,4);
  stinger_connected_components_stats stats;
  stinger_scc_reset_stats(&stats);

  const int batch_size = 2;
  int64_t batch[batch_size*2];
  batch[0] = 3;
  batch[1] = 8;
  batch[2] = 1;
  batch[3] = 2;

  stinger_insert_edge_pair(S, 0, batch[0], batch[1], 1, 2);
  // stinger_insert_edge_pair(S, 0, batch[1], batch[0], 1, 2);
  stinger_remove_edge_pair(S, 0, batch[2], batch[3]);
  // stinger_remove_edge_pair(S, 0, batch[3], batch[2]);

  batch[2] = ~1;
  batch[3] = ~2;

  stinger_scc_update(S,nv,scc_internal,&stats,batch,batch_size);

  const int64_t* currComponents = stinger_scc_get_components(scc_internal);

  int64_t* expected_components = (int64_t*)malloc(sizeof(int64_t)*nv);
  int64_t num_components = parallel_shiloach_vishkin_components_of_type(S, expected_components,0);

  for(uint64_t v = 0; v < nv; v++) {
    uint64_t Cp_shiloach = expected_components[v];
    EXPECT_NEAR(currComponents[v],currComponents[Cp_shiloach],0.00001) << "v = " << v;
  }
    free(expected_components);
}

int main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

