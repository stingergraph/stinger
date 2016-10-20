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

typedef std::vector<stinger_edge_update>::iterator update_iterator;

class StingerBatchTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
    stinger_config->nv = 1<<13;
    stinger_config->nebs = 1<<16;
    stinger_config->netypes = 1;
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

TEST_F(StingerBatchTest, single_insertion) {
    int64_t total_edges = 0;

    // Create batch to insert
    std::vector<stinger_edge_update> updates;
    stinger_edge_update u = {
        0, NULL, // type, type_str
        4, NULL, // source, source_str
        5, NULL, // destination, destination_str
        1, // weight
        100, // time
        0, 0 // result, meta_index
    };
    updates.push_back(u);

    stinger_batch_update(S, updates, EDGE_WEIGHT_INCR);

    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);
}


TEST_F(StingerBatchTest, simple_batch_insertion) {
    int64_t total_edges = 0;

    // Create batch to insert
    std::vector<stinger_edge_update> updates;
    for (int i = 0; i < 2; ++i)
    {
        stinger_edge_update u = {
            0, NULL, // type, type_str
            i, NULL, // source, source_str
            i+1, NULL, // destination, destination_str
            1, // weight
            i*100, // time
            0, 0 // result, meta_indesx
        };
        updates.push_back(u);
    }

    // Insert the new way
    stinger_batch_update(S, updates, EDGE_WEIGHT_INCR);

    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);
}

TEST_F(StingerBatchTest, batch_insertion) {
    int64_t total_edges = 0;
    int64_t consistency;

    // Create batch to insert
    std::vector<stinger_edge_update> updates;
    for (int i = 0; i < 100; ++i)
    {
        stinger_edge_update u = {
            0, NULL, // type, type_str
            i, NULL, // source, source_str
            i+1, NULL, // destination, destination_str
            1, // weight
            i*100, // time
            0, 0 // result, meta_index
        };
        updates.push_back(u);
        stinger_edge_update v = {
            0, NULL, // type, type_str
            i+1, NULL, // source, source_str
            i, NULL, // destination, destination_str
            1, // weight
            i*100, // time
            0, 0 // result, meta_index
        };
        updates.push_back(v);
    }

    // Do the updates
    stinger_batch_update(S, updates, EDGE_WEIGHT_INCR);

    // Make sure all edges were detected as "new"
    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    // Reset return codes so we can reuse this batch
    OMP("omp parallel for")
    for (update_iterator u = updates.begin(); u < updates.end(); ++u) { u->result = 0; }

    // Insert same batch again
    stinger_batch_update(S, updates, EDGE_WEIGHT_INCR);

    // Make sure all edges were detected as "updated"
    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 0);
    }

    // Make sure all edge weights were incremented properly
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
        EXPECT_EQ(STINGER_EDGE_WEIGHT, 2);
    }STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

}


int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
