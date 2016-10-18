#include "stinger_batch_test.h"
extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_traversal.h"
  #include "stinger_core/stinger_atomics.h"
  #include "stinger_net/stinger_alg.h"
}
#include "stinger_core/stinger_batch_insert.h"

#include <vector>
#include <scc_test.h>

class StingerBatchTest : public ::testing::Test {
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

TEST_F(StingerBatchTest, stinger_batch_edge_insertion) {
    int64_t total_edges = 0;


    // Create batch to insert
    std::vector<stinger_edge_update> updates;
    for (int i = 0; i < updates.size(); ++i)
    {
        stinger_edge_update u = {
            0, NULL, // type, type_str
            i, NULL, // source, source_str
            i+1, NULL, // destination, destination_str
            1, // weight
            i*100, // times
            0, 0 // result, meta_index
        };
        updates.push_back(u);
    }

    // Insert them into the graph, the old way
    OMP("parallel for")
    for (std::vector<stinger_edge_update>::iterator f = updates.begin(); f != updates.end(); ++f)
    {
        stinger_incr_edge(S, f->type, f->source, f->destination, f->weight, f->time);
    }

    // Insert the new way
    stinger_batch_update(S, updates, STINGER_EDGE_DIRECTION_OUT, EDGE_WEIGHT_INCR);


    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);
}


int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
