//
// Created by jdeeb3 on 6/6/16.
//
#include "independent_sets_test.h"

#define restrict

class IndependentTest : public ::testing::Test {
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

TEST_F(IndependentTest, simple_directed) {
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
    int64_t * iset = (int64_t *)xcalloc(nv, sizeof(int64_t));
    independent_set(S, nv, iset);

    for(int64_t v = 0; v<nv; v++){
        if (iset[v]){
            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v){
                                        if(iset[STINGER_EDGE_DEST]){
                                            //FAIL
                                            printf("%ld => %ld \n", v, STINGER_EDGE_DEST);
                                            ASSERT_EQ(0,1);
                                        }
                                    }STINGER_FORALL_OUT_EDGES_OF_VTX_END();
            STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(S, v){
                                        if(iset[STINGER_EDGE_DEST]){
                                            //FAIL
                                            printf("%ld => %ld \n", v, STINGER_EDGE_DEST);
                                            ASSERT_EQ(0,1);
                                        }
                                    }STINGER_FORALL_IN_EDGES_OF_VTX_END();
        }
    }
}

TEST_F(IndependentTest, medium_DAG) {
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

    int64_t nv = stinger_max_active_vertex(S) + 1;
    int64_t * iset = (int64_t *)xcalloc(nv, sizeof(int64_t));
    independent_set(S, nv, iset);

    for(int64_t v = 0; v<nv; v++){
        if (iset[v]){
            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v){
                                        if(iset[STINGER_EDGE_DEST]){
                                            //FAIL
                                            //printf("%ld => %ld \n", v, STINGER_EDGE_DEST);
                                            ASSERT_EQ(0,1);
                                        }
                                    }STINGER_FORALL_OUT_EDGES_OF_VTX_END();
            STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(S, v){
                                        if(iset[STINGER_EDGE_DEST]){
                                            //FAIL
                                            //printf("%ld => %ld \n", v, STINGER_EDGE_DEST);
                                            ASSERT_EQ(0,1);
                                        }
                                    }STINGER_FORALL_IN_EDGES_OF_VTX_END();
        }
    }
}

int
main (int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}