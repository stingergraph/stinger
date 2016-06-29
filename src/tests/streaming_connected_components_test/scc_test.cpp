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

  stinger_edge_update insertion,deletion;
  insertion.source       = 3;
  insertion.destination  = 8;
  deletion.source        = 1;
  deletion.destination   = 2;

  stinger_insert_edge_pair(S, 0, insertion.source, insertion.destination, 1, 2);
  stinger_remove_edge_pair(S, 0, deletion.source, deletion.destination);

  stinger_scc_insertion(S,nv,scc_internal,&stats,&insertion,1);
  stinger_scc_deletion(S,nv,scc_internal,&stats,&deletion,1);

  const int64_t* actual_components = stinger_scc_get_components(scc_internal);

  int64_t* expected_components = (int64_t*)malloc(sizeof(int64_t)*nv);
  parallel_shiloach_vishkin_components_of_type(S, expected_components,0);

  uint64_t expected_num_components = 0,actual_num_components=0;
  for(uint64_t v = 0; v < nv; v++) {
    if (v==expected_components[v])
      expected_num_components++;
    if (v==actual_components[v])
      actual_num_components++;
  }  
  EXPECT_EQ(actual_num_components,expected_num_components);

  for(uint64_t v = 0; v < nv; v++) {
    EXPECT_EQ(actual_components[v],expected_components[v]);
  }

    free(expected_components);

  stinger_scc_release_internals(&scc_internal);  
  
}



TEST_F(SCCTest, UndirectedGraphDumbbell) {
  stinger_insert_edge_pair(S, 0, 0, 1, 1, 1);
  stinger_insert_edge_pair(S, 0, 0, 2, 1, 1);
  stinger_insert_edge_pair(S, 0, 0, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 1, 2, 1, 1);
  stinger_insert_edge_pair(S, 0, 1, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 2, 3, 1, 1);

  stinger_insert_edge_pair(S, 0, 3, 4, 1, 1);

  stinger_insert_edge_pair(S, 0, 4, 5, 1, 1);
  stinger_insert_edge_pair(S, 0, 4, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 4, 7, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 7, 1, 1);

  int64_t nv = stinger_max_nv(S);
  stinger_scc_internal scc_internal;
  stinger_scc_initialize_internals(S,nv,&scc_internal,4);
  stinger_connected_components_stats stats;
  stinger_scc_reset_stats(&stats);

  stinger_edge_update insertion,deletion;
  insertion.source       = 6;
  insertion.destination  = 7;
  deletion.source        = 3;
  deletion.destination   = 4;

  stinger_insert_edge_pair(S, 0, insertion.source, insertion.destination, 1, 2);
  stinger_remove_edge_pair(S, 0, deletion.source, deletion.destination);

  stinger_scc_insertion(S,nv,scc_internal,&stats,&insertion,1);
  stinger_scc_deletion(S,nv,scc_internal,&stats,&deletion,1);

  const int64_t* actual_components = stinger_scc_get_components(scc_internal);

  int64_t* expected_components = (int64_t*)malloc(sizeof(int64_t)*nv);
  parallel_shiloach_vishkin_components_of_type(S, expected_components,0);
  uint64_t expected_num_components = 0,actual_num_components=0;
  for(uint64_t v = 0; v < nv; v++) {
    if (v==expected_components[v])
      expected_num_components++;
    if (v==actual_components[v])
      actual_num_components++;
  }  
  EXPECT_EQ(actual_num_components,expected_num_components);

  for(uint64_t v = 0; v < nv; v++) {
    EXPECT_EQ(actual_components[v],expected_components[v]);
  }
  free(expected_components);

  stinger_scc_release_internals(&scc_internal);  
}

int main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


