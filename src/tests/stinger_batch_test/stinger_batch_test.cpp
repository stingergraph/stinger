#include "stinger_batch_test.h"
extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_traversal.h"
  #include "stinger_core/stinger_atomics.h"
}
#include "stinger_core/stinger_batch_insert.h"

#include <vector>

struct update {
    int64_t type;
    int64_t source;
    int64_t destination;
    int64_t weight;
    int64_t time;
    int64_t result;
    static int64_t get_type(const update &u) { return u.type; }
    static void set_type(update &u, int64_t v) { u.type = v; }
    static int64_t get_source(const update &u) { return u.source; }
    static void set_source(update &u, int64_t v) { u.source = v; }
    static int64_t get_dest(const update &u) { return u.destination; }
    static void set_dest(update &u, int64_t v) { u.destination = v; }
    static int64_t get_weight(const update &u) { return u.weight; }
    static int64_t get_time(const update &u) { return u.time; }
    static int64_t get_result(const update& u) { return u.result; }
    static int64_t set_result(update &u, int64_t v) { u.result = v; }
};

typedef std::vector<update>::iterator update_iterator;

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
    std::vector<update> updates;
    update u = {
        0, // type
        4, // source,
        5, // destination
        1, // weight
        100, // time
        0 // result
    };
    updates.push_back(u);
    stinger_batch_incr_edges<update>(S, updates.begin(), updates.end());

    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    int64_t num_edges = 0;
    STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, 4){
        EXPECT_EQ(STINGER_EDGE_DEST, 5);
        ++num_edges;
    }STINGER_FORALL_OUT_EDGES_OF_VTX_END();

    EXPECT_EQ(stinger_outdegree_get(S, 4), 1);
    EXPECT_EQ(stinger_indegree_get(S, 5), 1);

    EXPECT_EQ(num_edges, 1);

    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);
}


TEST_F(StingerBatchTest, simple_batch_insertion) {
    int64_t total_edges = 0;

    // Create batch to insert
    std::vector<update> updates;
    for (int i = 0; i < 2; ++i)
    {
        update u = {
            0, // type
            i, // source
            i+1, // destination
            1, // weight
            i*100, // time
            0 // result
        };
        updates.push_back(u);
    }

    stinger_batch_incr_edges<update>(S, updates.begin(), updates.end());

    int64_t consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    for (update_iterator u = updates.begin(); u != updates.end(); ++u)
    {
        EXPECT_EQ(u->result, 1);
    }

    for (int i = 0; i < 2; ++i){
        int64_t num_edges = 0;
        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, i){
            EXPECT_EQ(STINGER_EDGE_DEST, i+1);
            ++num_edges;
        }STINGER_FORALL_OUT_EDGES_OF_VTX_END();
        EXPECT_EQ(num_edges, 1);
    }

    EXPECT_EQ(stinger_outdegree_get(S, 0), 1);
    EXPECT_EQ(stinger_indegree_get(S, 0), 0);

    EXPECT_EQ(stinger_outdegree_get(S, 1), 1);
    EXPECT_EQ(stinger_indegree_get(S, 1), 1);

    EXPECT_EQ(stinger_outdegree_get(S, 2), 0);
    EXPECT_EQ(stinger_indegree_get(S, 2), 1);

}

TEST_F(StingerBatchTest, batch_insertion) {
    int64_t total_edges = 0;
    int64_t consistency;

    // Create batch to insert
    std::vector<update> updates;
    for (int i=0; i < 100; i++) {
        int64_t timestamp = i+1;
        for (int j=i+1; j < 100; j++) {
            update u = {
                0, // type
                i, // source
                j, // destination
                1, // weight
                timestamp, // time
                0, // result
            };
            updates.push_back(u);
        }
    }

    // Do the updates
    stinger_batch_incr_edges<update>(S, updates.begin(), updates.end());

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
    stinger_batch_incr_edges<update>(S, updates.begin(), updates.end());

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

    for (int i=0; i < 100; i++) {
        EXPECT_EQ(stinger_outdegree_get(S, i), 99 - i);
    }
}

TEST_F(StingerBatchTest, duplicate_batch_insertion) {
    int64_t total_edges = 0;
    int64_t consistency;

    // Create batch to insert
    std::vector<update> updates;

    const int num_dupes = 10;
    int total_weight_per_edge = 0;
    int64_t num_edges;
    for (int d=0; d < num_dupes; ++d) {
        num_edges = 0;
        for (int i=0; i < 100; i++) {
            for (int j=i+1; j < 100; j++) {
                update u = {
                    0, // type
                    i, // source
                    j, // destination
                    d, // weight
                    d, // time
                    0  // result
                };
                updates.push_back(u);
                num_edges++;
            }
        }
        total_weight_per_edge += d;
    }

    // Do the updates
    stinger_batch_incr_edges<update>(S, updates.begin(), updates.end());

    consistency = stinger_consistency_check(S,S->max_nv);
    EXPECT_EQ(consistency,0);

    // Only one update for each edge should return as "added", the rest should indicate "updated"
    int64_t num_inserts = 0;
    for (update_iterator u = updates.begin(); u < updates.end(); ++u)
    {
        if (u->result == 1) { ++num_inserts; }
    }
    EXPECT_EQ(num_inserts, num_edges);

    // Weight should be sum of all updates
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
        EXPECT_EQ(STINGER_EDGE_WEIGHT, total_weight_per_edge);
    }STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

    for (int i=0; i < 100; i++) {
        EXPECT_EQ(stinger_outdegree_get(S, i), 99 - i);
    }
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
