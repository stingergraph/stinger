//
// Created by jdeeb3 on 6/3/16.
//

#include "shortest_paths_test.h"

#define restrict

class ShortestPathsTest : public ::testing::Test {
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

TEST_F(ShortestPathsTest, simple_directed) {
    stinger_insert_edge(S, 0, 0, 1, 1, 1);
    stinger_insert_edge(S, 0, 0, 4, 1, 1);
    stinger_insert_edge(S, 0, 1, 3, 10, 1);
    stinger_insert_edge(S, 0, 2, 1, 1, 1);
    stinger_insert_edge(S, 0, 3, 2, 1, 1);
    stinger_insert_edge(S, 0, 4, 2, 1, 1);
    stinger_insert_edge(S, 0, 4, 5, 1, 1);
    stinger_insert_edge(S, 0, 5, 6, 1, 1);
    stinger_insert_edge(S, 0, 6, 4, 1, 1);

    int64_t nv = stinger_max_active_vertex(S)+1;
    int64_t path_length = a_star(S, nv, 5, 2, true);
    EXPECT_EQ(3,path_length);

    path_length = a_star(S, nv, 2, 3, true);
    EXPECT_EQ(2,path_length);

    path_length = a_star(S, nv, 5, 0, true);
    EXPECT_EQ(std::numeric_limits<int64_t>::max(),path_length);

}

TEST_F(ShortestPathsTest, medium_DAG){
    stinger_insert_edge(S, 0, 0, 6, 1, 1);
    stinger_insert_edge(S, 0, 1, 2, 10, 1);
    stinger_insert_edge(S, 0, 1, 3, 10, 1);
    stinger_insert_edge(S, 0, 1, 4, 10, 1);
    stinger_insert_edge(S, 0, 1, 5, 10, 1);
    stinger_insert_edge(S, 0, 1, 6, 10, 1);
    stinger_insert_edge(S, 0, 2, 7, 9, 1);
    stinger_insert_edge(S, 0, 2, 8, 9, 1);
    stinger_insert_edge(S, 0, 2, 9, 9, 1);
    stinger_insert_edge(S, 0, 3, 10, 8, 1);
    stinger_insert_edge(S, 0, 3, 11, 8, 1);
    stinger_insert_edge(S, 0, 4, 12, 8, 1);
    stinger_insert_edge(S, 0, 8, 18, 7, 1);
    stinger_insert_edge(S, 0, 9, 16, 6, 1);
    stinger_insert_edge(S, 0, 9, 17, 6, 1);
    stinger_insert_edge(S, 0, 10, 15, 5, 1);
    stinger_insert_edge(S, 0, 11, 14, 5, 1);
    stinger_insert_edge(S, 0, 12, 13, 5, 1);

    int64_t nv = stinger_max_active_vertex(S)+1;
    std::vector<int64_t> paths;
    paths = dijkstra(S, nv, 1, false);

    EXPECT_EQ(7+9+10, paths[18]);

    paths = dijkstra(S, nv, 1, true);
    EXPECT_EQ(3, paths[18]);
}



int
main (int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}