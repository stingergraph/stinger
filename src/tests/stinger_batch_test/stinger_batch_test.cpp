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

    stinger_batch_incr_edge(S, updates);

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
    stinger_batch_incr_edge(S, updates);

    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }
}

TEST_F(StingerBatchTest, batch_insertion) {
    int64_t total_edges = 0;
    int64_t consistency;

    // Create batch to insert
    std::vector<stinger_edge_update> updates;
    for (int i=0; i < 100; i++) {
        int64_t timestamp = i+1;
        for (int j=i+1; j < 100; j++) {
            stinger_edge_update u = {
                0, NULL, // type, type_str
                i, NULL, // source, source_str
                j, NULL, // destination, destination_str
                1, // weight
                timestamp, // time
                0, 0 // result, meta_index
            };
            updates.push_back(u);
        }
    }

    // Do the updates
    stinger_batch_incr_edge(S, updates);

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    // Make sure all edges were detected as "new"
    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    // Reset return codes so we can reuse this batch
    OMP("omp parallel for")
    for (update_iterator u = updates.begin(); u < updates.end(); ++u) { u->result = 0; }

    // Insert same batch again
    stinger_batch_incr_edge(S, updates);

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    // Make sure all edges were detected as "updated"
    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 0);
    }

    // Make sure all edge weights were incremented properly
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
        EXPECT_EQ(STINGER_EDGE_WEIGHT, 2);
    }STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
}

TEST_F(StingerBatchTest, duplicate_batch_insertion) {
    int64_t total_edges = 0;
    int64_t consistency;

    // Create batch to insert
    std::vector<stinger_edge_update> updates;

    const int num_dupes = 10;
    int total_weight_per_edge = 0;
    int64_t num_edges;
    for (int d=0; d < num_dupes; ++d) {
        num_edges = 0;
        for (int i=0; i < 100; i++) {
            for (int j=i+1; j < 100; j++) {
                stinger_edge_update u = {
                    0, NULL, // type, type_str
                    i, NULL, // source, source_str
                    j, NULL, // destination, destination_str
                    d, // weight
                    d, // time
                    0, 0 // result, meta_index
                };
                updates.push_back(u);
                num_edges++;
            }
        }
        total_weight_per_edge += d;
    }

    // Do the updates
    stinger_batch_incr_edge(S, updates);

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    // Only one update for each edge should return as "added", the rest should indicate "updated"
    int64_t num_inserts = 0;
    for (update_iterator u = updates.begin(); u < updates.end(); ++u)
    {
        if (u->result == 1) { ++num_inserts; }
    }
    EXPECT_EQ(num_inserts, num_edges);

    // Timestamp should reflect the last update
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
        EXPECT_EQ(STINGER_EDGE_TIME_RECENT, num_dupes - 1);
    }STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

    // Weight should be sum of all updates
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
        EXPECT_EQ(STINGER_EDGE_WEIGHT, total_weight_per_edge);
    }STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
