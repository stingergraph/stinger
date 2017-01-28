#ifndef STINGER_TRAVERSAL_TEST_H_
#define STINGER_TRAVERSAL_TEST_H_

extern "C" {
  #include "stinger_core/stinger.h" 
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_atomics.h"
  #include "stinger_core/stinger_traversal.h"
}

#include "gtest/gtest.h"

class StingerTraversalTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
    stinger_config->nv = 1<<13;
    stinger_config->nebs = 1<<16;
    stinger_config->netypes = 2;
    stinger_config->nvtypes = 3;
    stinger_config->memory_size = 1<<30;
    stinger_config->no_map_none_vtype = true;
    S = stinger_new_full(stinger_config);
    xfree(stinger_config);

    int64_t timestamp = 1;

    for (int i=0; i < 10; i++) {
      for (int j=i+1; j < 10; j++) {
        int64_t type = (j % 2 == 1)?1:0;
        (void) stinger_insert_edge_pair(S, type, i, j, 1, timestamp);
        expected_out_edges[std::make_pair(type,i)].insert(j);
        expected_out_edges[std::make_pair(type,j)].insert(i);

        expected_in_edges[std::make_pair(type,j)].insert(i);
        expected_in_edges[std::make_pair(type,i)].insert(j);
      }
    }
    
    for (int i=0; i < 100; i++) {
      for (int j=i+101; j < 200; j++) {
        stinger_insert_edge(S, 0, j, i, 1, timestamp);
        expected_out_edges[std::make_pair(0,j)].insert(i);
        
        expected_in_edges[std::make_pair(0,i)].insert(j);
      }
    }
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> > expected_out_edges;
  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> > expected_in_edges;
};


#endif /* STINGER_TRAVERSAL_TEST_H_ */
