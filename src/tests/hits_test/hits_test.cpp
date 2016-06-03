//
// Created by jdeeb3 on 6/7/16.
//

#include "hits_test.h"

#define restrict

class HITSTest : public ::testing::Test {
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

TEST_F(HITSTest, UndirectedLoop) {
    stinger_insert_edge(S, 0, 0, 1, 1, 1);
    stinger_insert_edge(S, 0, 0, 8, 1, 1);
    stinger_insert_edge(S, 0, 1, 0, 1, 1);
    stinger_insert_edge(S, 0, 1, 2, 1, 1);
    stinger_insert_edge(S, 0, 2, 1, 1, 1);
    stinger_insert_edge(S, 0, 2, 3, 1, 1);
    stinger_insert_edge(S, 0, 3, 2, 1, 1);
    stinger_insert_edge(S, 0, 3, 4, 1, 1);
    stinger_insert_edge(S, 0, 4, 3, 1, 1);
    stinger_insert_edge(S, 0, 4, 5, 1, 1);
    stinger_insert_edge(S, 0, 5, 4, 1, 1);
    stinger_insert_edge(S, 0, 5, 6, 1, 1);
    stinger_insert_edge(S, 0, 6, 5, 1, 1);
    stinger_insert_edge(S, 0, 6, 7, 1, 1);
    stinger_insert_edge(S, 0, 7, 6, 1, 1);
    stinger_insert_edge(S, 0, 7, 8, 1, 1);
    stinger_insert_edge(S, 0, 8, 7, 1, 1);
    stinger_insert_edge(S, 0, 8, 0, 1, 1);

    int64_t nv = stinger_max_active_vertex(S)+1;

    double_t * auth = (double_t *)xcalloc(nv, sizeof(double_t));
    double_t * hubs = (double_t *)xcalloc(nv, sizeof(double_t));

    hits_centrality(S, nv, hubs, auth, 10);

    for (int64_t v = 1; v< nv; v++){
        if (auth[v-1] != auth[v]){
            EXPECT_EQ(0,1);
        }
        if (hubs[v-1] != hubs[v]){
            EXPECT_EQ(0,1);
        }
    }

}

TEST_F(HITSTest, DirectedBipartite) {
    stinger_insert_edge(S, 0, 0, 5, 1, 1);
    stinger_insert_edge(S, 0, 0, 6, 1, 1);
    stinger_insert_edge(S, 0, 0, 7, 1, 1);
    stinger_insert_edge(S, 0, 0, 8, 1, 1);
    stinger_insert_edge(S, 0, 0, 9, 1, 1);
    stinger_insert_edge(S, 0, 1, 5, 1, 1);
    stinger_insert_edge(S, 0, 1, 6, 1, 1);
    stinger_insert_edge(S, 0, 1, 7, 1, 1);
    stinger_insert_edge(S, 0, 1, 8, 1, 1);
    stinger_insert_edge(S, 0, 1, 9, 1, 1);
    stinger_insert_edge(S, 0, 2, 5, 1, 1);
    stinger_insert_edge(S, 0, 2, 6, 1, 1);
    stinger_insert_edge(S, 0, 2, 7, 1, 1);
    stinger_insert_edge(S, 0, 2, 8, 1, 1);
    stinger_insert_edge(S, 0, 2, 9, 1, 1);
    stinger_insert_edge(S, 0, 3, 5, 1, 1);
    stinger_insert_edge(S, 0, 3, 6, 1, 1);
    stinger_insert_edge(S, 0, 3, 7, 1, 1);
    stinger_insert_edge(S, 0, 3, 8, 1, 1);
    stinger_insert_edge(S, 0, 3, 9, 1, 1);
    stinger_insert_edge(S, 0, 4, 5, 1, 1);
    stinger_insert_edge(S, 0, 4, 6, 1, 1);
    stinger_insert_edge(S, 0, 4, 7, 1, 1);
    stinger_insert_edge(S, 0, 4, 8, 1, 1);
    stinger_insert_edge(S, 0, 4, 9, 1, 1);


    int64_t nv = stinger_max_active_vertex(S)+1;

    double_t * auth = (double_t *)xcalloc(nv, sizeof(double_t));
    double_t * hubs = (double_t *)xcalloc(nv, sizeof(double_t));

    hits_centrality(S, nv, hubs, auth, 100);

    for (int64_t v = 1; v< 5; v++){
        if (auth[v-1] != auth[v]){
            EXPECT_EQ(0,1);
        }
        if (hubs[v-1] != hubs[v]){
            EXPECT_EQ(0,1);
        }
    }
    for (int64_t v = 6; v< 10; v++){
        if (auth[v-1] != auth[v]){
            EXPECT_EQ(0,1);
        }
        if (hubs[v-1] != hubs[v]){
            EXPECT_EQ(0,1);
        }
    }

}


int
main (int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}