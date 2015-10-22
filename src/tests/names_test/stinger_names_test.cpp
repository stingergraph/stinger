
#include "stinger_names_test.h"


class StingerNamesTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    stinger_names = stinger_names_new(5);
  }

  virtual void TearDown() {
    free(stinger_names);
  }

  stinger_names_t * stinger_names;
};

TEST_F(StingerNamesTest, create_names) {
  int64_t out = 0;
  
  char * test_names [] = {
      "Alpha",
      "Beta",
      "Gamma",
      "Sigma",
      "Zeta"
  };

  for (int i=0; i < 5; i++) {
    int64_t expected_out;
    int64_t expected_status;
    expected_out = i;
    expected_status = 1;

    int64_t status = stinger_names_create_type(stinger_names, test_names[i], &out);
    EXPECT_EQ(expected_status,status);
    EXPECT_EQ(expected_out, out);
  }
}

TEST_F(StingerNamesTest, overflow_create_names) {
  int64_t out = 0;
  
  char * test_names [] = {
      "Alpha",
      "Beta",
      "Gamma",
      "Sigma",
      "Zeta",
      "Tau",
      "Pi"
  };

  for (int i=0; i < 7; i++) {
    int64_t expected_out;
    int64_t expected_status;
    if (i >= 5) {
      expected_out = -1;
      expected_status = -1;
    } else {
      expected_out = i;
      expected_status = 1;
    }

    int64_t status = stinger_names_create_type(stinger_names, test_names[i], &out);
    EXPECT_EQ(expected_status,status);
    EXPECT_EQ(expected_out, out);
  }
}

TEST_F(StingerNamesTest, create_duplicate_name) {
  int64_t out = 0;
  int64_t status;

  int64_t expected_out;
  int64_t expected_status;

  expected_out = 0;
  expected_status = 1;

  status = stinger_names_create_type(stinger_names, "Alpha", &out);
  EXPECT_EQ(expected_status,status);
  EXPECT_EQ(expected_out, out);

  expected_out = 0;
  expected_status = 0;

  status = stinger_names_create_type(stinger_names, "Alpha", &out);
  EXPECT_EQ(expected_status,status);
  EXPECT_EQ(expected_out, out);
}

TEST_F(StingerNamesTest, empty_lookup) {
  int64_t out = 0;
 
  out = stinger_names_lookup_type(stinger_names, "Alpha");
  EXPECT_EQ(-1, out);
}

TEST_F(StingerNamesTest, lookup_success) {
  int64_t out = 0;
  int64_t status;

  status = stinger_names_create_type(stinger_names, "Alpha", &out);
  EXPECT_EQ(1,status);
  EXPECT_EQ(0, out);

  status = stinger_names_create_type(stinger_names, "Beta", &out);
  EXPECT_EQ(1,status);
  EXPECT_EQ(1, out);
 
  out = stinger_names_lookup_type(stinger_names, "Alpha");
  EXPECT_EQ(0, out);

  out = stinger_names_lookup_type(stinger_names, "Beta");
  EXPECT_EQ(1, out);
}

TEST_F(StingerNamesTest, lookup_failure) {
  int64_t out = 0;
  int64_t status;

  status = stinger_names_create_type(stinger_names, "Alpha", &out);
  EXPECT_EQ(1,status);
  EXPECT_EQ(0, out);
 
  out = stinger_names_lookup_type(stinger_names, "Beta");
  EXPECT_EQ(-1, out);
}

TEST_F(StingerNamesTest, resize) {
  int64_t out = 0;
  
  char * test_names [] = {
      "Alpha",
      "Beta",
      "Gamma",
      "Sigma",
      "Zeta",
      "Tau",
      "Pi"
  };

  for (int i=0; i < 7; i++) {
    int64_t expected_out;
    int64_t expected_status;

    if (i == 5) {
      stinger_names_resize(&stinger_names, 7);
    }
    expected_out = i;
    expected_status = 1;

    int64_t status = stinger_names_create_type(stinger_names, test_names[i], &out);
    EXPECT_EQ(expected_status,status);
    EXPECT_EQ(expected_out, out);
  }

  for (int i=0; i < 7; i++) {
    int64_t expected_out;

    expected_out = i;

    out = stinger_names_lookup_type(stinger_names, test_names[i]);
    EXPECT_EQ(expected_out, out);
  }
}

int
main (int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



