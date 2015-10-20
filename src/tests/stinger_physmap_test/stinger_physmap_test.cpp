#include "stinger_physmap_test.h"
extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_traversal.h"
  #include "stinger_core/stinger_atomics.h"
}

class StingerPhysmapTest : public ::testing::Test {
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
  }

  virtual void TearDown() {
    stinger_free_all(S);
  }

  struct stinger_config_t * stinger_config;
  struct stinger * S;
};

TEST_F(StingerPhysmapTest, create) {
  int64_t vtx_out;
  int64_t ret;
  int64_t i;

  ret = stinger_mapping_nv(S);
  EXPECT_EQ(0,ret);

  char * name = (char*)xcalloc(100,sizeof(char));

  for (i=0; i < (1<<13); i++) {
    snprintf(name, 100, "Vertex_%ld", i);
    ret = stinger_mapping_create(S, name, strlen(name), &vtx_out);
    EXPECT_EQ(ret,1);
    EXPECT_EQ(vtx_out,i);    
  }
  ret = stinger_mapping_nv(S);
  EXPECT_EQ(1<<13,ret);

  ret = stinger_mapping_create(S, "Pi", 2, &vtx_out);
  EXPECT_EQ(ret,-1);

  ret = stinger_mapping_create(S, "Vertex_100", 10, &vtx_out);
  EXPECT_EQ(ret,0);
  EXPECT_EQ(vtx_out,100); 

  xfree(name);
}

TEST_F(StingerPhysmapTest, lookup) {
  vindex_t vtx;
  int64_t vtx_out;
  int64_t i;

  vtx = stinger_mapping_lookup(S, "Vertex_101", 10);
  EXPECT_EQ(vtx,-1);

  char * name = (char*)xcalloc(100,sizeof(char));

  for (i=0; i < (1<<13); i++) {
    snprintf(name, 100, "Vertex_%ld", i);
    stinger_mapping_create(S, name, strlen(name), &vtx_out);
  }

  xfree(name);

  vtx = stinger_mapping_lookup(S, "Vertex_101", 10);
  EXPECT_EQ(vtx,101);

  vtx = stinger_mapping_lookup(S, "Unknown_Vertex", 14);
  EXPECT_EQ(vtx,-1);
}

TEST_F(StingerPhysmapTest, physid_lookup) {
  int64_t ret;
  int64_t vtx_out;
  int64_t i;
  uint64_t len;

  char * name = (char*)xcalloc(100,sizeof(char));

  for (i=0; i < (1<<13); i++) {
    snprintf(name, 100, "Vertex_%ld", i);
    stinger_mapping_create(S, name, strlen(name), &vtx_out);
  }

  xfree(name); 

  char * lookupName = NULL;
  len = 0;

  ret = stinger_mapping_physid_get(S, 150, &lookupName, &len);
  EXPECT_EQ(ret,0);
  EXPECT_EQ(len,10);
  ASSERT_STREQ("Vertex_150", lookupName);

  free(lookupName);

  lookupName = (char *)xcalloc(11,sizeof(char));

  len = 11;
  ret = stinger_mapping_physid_get(S, 150, &lookupName, &len);
  EXPECT_EQ(ret,0);
  EXPECT_EQ(len,10);
  ASSERT_STREQ("Vertex_150", lookupName);

  free(lookupName);

  lookupName = (char *)xcalloc(4,sizeof(char));

  len = 4;
  ret = stinger_mapping_physid_get(S, 150, &lookupName, &len);
  EXPECT_EQ(ret,0);
  EXPECT_EQ(len,10);
  ASSERT_STREQ("Vertex_150", lookupName);

  free(lookupName);

  lookupName = NULL;

  len = 0;
  ret = stinger_mapping_physid_get(S, 1<<13 + 1, &lookupName, &len);
  EXPECT_EQ(ret,-1);
  EXPECT_EQ(lookupName,(void*)NULL);
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}