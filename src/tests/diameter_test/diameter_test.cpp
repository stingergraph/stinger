//
// Created by jdeeb3 on 6/6/16.
//

#include <stinger_alg/diameter.h>
extern "C" {
#include "stinger_core/stinger.h"
}
#include "gtest/gtest.h"

#define restrict

class DiameterTest : public ::testing::Test {
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

TEST_F(DiameterTest, simple_directed) {
    stinger_insert_edge(S, 0, 0, 1, 1, 1);
    stinger_insert_edge(S, 0, 0, 4, 1, 1);
    stinger_insert_edge(S, 0, 1, 3, 1, 1);
    stinger_insert_edge(S, 0, 2, 1, 1, 1);
    stinger_insert_edge(S, 0, 3, 2, 1, 1);
    stinger_insert_edge(S, 0, 4, 2, 1, 1);
    stinger_insert_edge(S, 0, 4, 5, 1, 1);
    stinger_insert_edge(S, 0, 5, 6, 1, 1);
    stinger_insert_edge(S, 0, 6, 4, 1, 1);

    int64_t nv = stinger_max_active_vertex(S)+1;
    int64_t pDiam;
    int64_t eDiam;
    pDiam = pseudo_diameter(S,nv, 1);
    EXPECT_EQ(2,pDiam);
    pDiam = pseudo_diameter(S,nv, 0);
    EXPECT_EQ(4,pDiam);

    eDiam = exact_diameter(S, nv);
    EXPECT_EQ(5,eDiam);
}


int
main (int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
