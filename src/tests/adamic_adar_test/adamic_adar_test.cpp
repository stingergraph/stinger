#include "adamic_adar_test.h"

#define restrict

class AdamicAdarTest : public ::testing::Test {
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

    stinger_insert_edge_pair(S,0,0,1,1,1);
    stinger_insert_edge_pair(S,0,0,2,1,1);
    stinger_insert_edge_pair(S,0,0,3,1,1);
    stinger_insert_edge_pair(S,0,1,4,1,1);
    stinger_insert_edge_pair(S,0,2,4,1,1);
    stinger_insert_edge_pair(S,0,1,5,1,1);
    stinger_insert_edge_pair(S,0,1,6,1,1);
    stinger_insert_edge_pair(S,0,1,7,1,1);
    stinger_insert_edge_pair(S,0,3,8,1,1);
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
};

class AdamicAdarDirectedTest : public ::testing::Test {
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

    stinger_insert_edge(S,0,0,1,1,1);
    stinger_insert_edge_pair(S,0,0,2,1,1);
    stinger_insert_edge(S,0,0,3,1,1);
    stinger_insert_edge(S,0,1,4,1,1);
    stinger_insert_edge_pair(S,0,2,4,1,1);
    stinger_insert_edge(S,0,1,5,1,1);
    stinger_insert_edge(S,0,6,1,1,1);
    stinger_insert_edge_pair(S,0,1,7,1,1);
    stinger_insert_edge(S,0,8,3,1,1);
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
};

TEST_F(AdamicAdarTest, AllEdgeTypes) {
  int64_t * candidates = NULL;
  double * scores = NULL;
  int64_t len = adamic_adar(S, 0, -1, &candidates, &scores);

  for (int64_t i = 0; i < len; i++) {
    switch (candidates[i]) {
      case 0:
        ADD_FAILURE() << "Start Vertex should not be a candidate";
        break;
      case 1:
        ADD_FAILURE() << "Vertex 1 should not be a candidate";
        break;
      case 2:
        ADD_FAILURE() << "Vertex 2 should not be a candidate";
        break;
      case 3:
        ADD_FAILURE() << "Vertex 3 should not be a candidate";
        break;
      case 4:
        EXPECT_DOUBLE_EQ(1.0/log(2)+1/log(5),scores[i]);
        break;
      case 5:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 6:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 7:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 8:
        EXPECT_DOUBLE_EQ(1.0/log(2),scores[i]);
        break;
    }
  }

  xfree(candidates);
  xfree(scores);
}

TEST_F(AdamicAdarTest, OneEdgeType) {
  for (int i=0; i < 100; i++) {
    int64_t timestamp = i+1;
    for (int j=i+1; j < 100; j++) {
      stinger_insert_edge_pair(S, 1, i, j, 1, timestamp);
    }
  }

  int64_t * candidates = NULL;
  double * scores = NULL;
  int64_t len = adamic_adar(S, 0, 0, &candidates, &scores);

  for (int64_t i = 0; i < len; i++) {
    switch (candidates[i]) {
      case 0:
        ADD_FAILURE() << "Start Vertex should not be a candidate";
        break;
      case 1:
        ADD_FAILURE() << "Vertex 1 should not be a candidate";
        break;
      case 2:
        ADD_FAILURE() << "Vertex 2 should not be a candidate";
        break;
      case 3:
        ADD_FAILURE() << "Vertex 3 should not be a candidate";
        break;
      case 4:
        EXPECT_DOUBLE_EQ(1.0/log(2)+1/log(5),scores[i]);
        break;
      case 5:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 6:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 7:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 8:
        EXPECT_DOUBLE_EQ(1.0/log(2),scores[i]);
        break;
    }
  }

  xfree(candidates);
  xfree(scores);
}

TEST_F(AdamicAdarDirectedTest, AllEdgeTypes) {
  int64_t * candidates = NULL;
  double * scores = NULL;
  int64_t len = adamic_adar(S, 0, -1, &candidates, &scores);

  for (int64_t i = 0; i < len; i++) {
    switch (candidates[i]) {
      case 0:
        ADD_FAILURE() << "Start Vertex should not be a candidate";
        break;
      case 1:
        ADD_FAILURE() << "Vertex 1 should not be a candidate";
        break;
      case 2:
        ADD_FAILURE() << "Vertex 2 should not be a candidate";
        break;
      case 3:
        ADD_FAILURE() << "Vertex 3 should not be a candidate";
        break;
      case 4:
        EXPECT_DOUBLE_EQ(1.0/log(2)+1/log(5),scores[i]);
        break;
      case 5:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 6:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 7:
        EXPECT_DOUBLE_EQ(1.0/log(5),scores[i]);
        break;
      case 8:
        EXPECT_DOUBLE_EQ(1.0/log(2),scores[i]);
        break;
    }
  }

  xfree(candidates);
  xfree(scores);
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}