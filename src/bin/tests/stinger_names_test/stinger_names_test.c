/*
 * stinger_names_test.c
 *
 *  Created on: Oct 28, 2014
 *      Author: jason
 */

#include "stinger_names_test.h"

struct create_args {
  char * name;
  stinger_names_t ** stinger_names_p;
  int64_t expected_status;
  int64_t expected_mapping;
};

struct lookup_args {
  char * name;
  stinger_names_t ** stinger_names_p;
  int64_t expected_mapping;
};

struct resize_args {
  stinger_names_t ** stinger_names_p;
  int64_t new_size;
};

void test_stinger_names_create(test_harness_t * th, void * arg) {
  struct create_args * args = (struct create_args *)arg;
  char * name = args->name;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  int64_t out = 0;
  int64_t status = stinger_names_create_type(stinger_names, name, &out);
  TEST_ASSERT_EQ(status,args->expected_status);
  TEST_ASSERT_EQ(out,args->expected_mapping);
  if (FAILED_TEST) {
    fprintf(stderr,"Lookup %s\n", name);
    fprintf(stderr,"status - Observed: %lld, Expected: %lld\n", status, args->expected_status);
    fprintf(stderr,"mapping - Observed: %lld, Expected: %lld\n", out, args->expected_mapping);
  }
}

void test_stinger_names_resize(test_harness_t * th, void * arg) {
  struct resize_args * args = (struct resize_args *)arg;
  int64_t new_size = args->new_size;
  stinger_names_resize(args->stinger_names_p, new_size);
  stinger_names_t * stinger_names = *(args->stinger_names_p);
  TEST_ASSERT_NEQ(stinger_names,NULL);
  if (FAILED_TEST) {
    fprintf(stderr,"Failed to resize the stinger_names\n");
  }
  TEST_ASSERT_EQ(stinger_names->max_types, new_size);
  if (FAILED_TEST) {
    fprintf(stderr,"Resized stinger_names to %lld elements - Observed %lld elements\n", new_size, stinger_names->max_types);
  }
}

void test_stinger_names_lookup(test_harness_t * th, void * arg) {
  struct lookup_args * args = (struct lookup_args *)arg;
  char * name = args->name;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  int64_t out = -1;
  out = stinger_names_lookup_type(stinger_names, name);
  TEST_ASSERT_EQ(out,args->expected_mapping);
  if (FAILED_TEST) {
    fprintf(stderr,"Lookup %s\n", name);
    fprintf(stderr,"mapping - Observed: %lld, Expected: %lld\n", out, args->expected_mapping);
  }
}

int
main (const int argc, char *argv[])
{
  test_harness_t * th = new_test_harness(50);

  stinger_names_t * names = stinger_names_new(5);

  char * test_names [] = {
      "Alpha",
      "Beta",
      "Gamma",
      "Sigma",
      "Zeta",
      "Tau",
      "Pi"
  };

  // Lookup from Empty STINGER names
  register_test(th,
      "Empty stinger_names lookup",
      &test_stinger_names_lookup,
      (void*)&(struct lookup_args){test_names[0],&names,-1});

  // Create New types
  for (int i=0; i < 7; i++) {
    int64_t expected_out;
    int64_t expected_status;

    if (i < 5) {
      expected_out = i;
      expected_status = 1;
    } else {
      expected_out = -1;
      expected_status = -1;
    }

    struct create_args * create_args = (struct create_args *)malloc(sizeof(struct create_args));

    create_args->name = test_names[i];
    create_args->stinger_names_p = &names;
    create_args->expected_status = expected_status;
    create_args->expected_mapping = expected_out;

    register_test(th,
        "Create",
        &test_stinger_names_create,
        (void*)create_args);
  }

  // Lookup types
  for (int i=0; i < 7; i++) {
    int64_t expected_out;

    if (i < 5) {
      expected_out = i;
    } else {
      expected_out = -1;
    }

    struct lookup_args * lookup_args = (struct lookup_args *)malloc(sizeof(struct lookup_args));

    lookup_args->name = test_names[i];
    lookup_args->stinger_names_p = &names;
    lookup_args->expected_mapping = expected_out;

    register_test(th,
        "Lookup",
        &test_stinger_names_lookup,
        (void*)lookup_args);
  }

  // Try to re-create
  register_test(th,
      "Re-create",
      &test_stinger_names_create,
      (void*)&(struct create_args){test_names[0],&names, 0, 0});

  // Resize the atinger_names
   struct resize_args ra = {&names, 7};

  register_test(th,
        "Resize",
        &test_stinger_names_resize,
        (void*)&ra);

  // Lookup the inserted names from before
  for (int i=0; i < 7; i++) {
    int64_t expected_out;

    if (i < 5) {
      expected_out = i;
    } else {
      expected_out = -1;
    }

    struct lookup_args * lookup_args = (struct lookup_args *)malloc(sizeof(struct lookup_args));

    lookup_args->name = test_names[i];
    lookup_args->stinger_names_p = &names;
    lookup_args->expected_mapping = expected_out;

    register_test(th,
        "Lookup",
        &test_stinger_names_lookup,
        (void*)lookup_args);
  }

  // Try additional creation
  for (int i=0; i < 7; i++) {
     int64_t expected_out;
     int64_t expected_status;

     if (i < 5) {
       expected_out = i;
       expected_status = 0;
     } else {
       expected_out = i;
       expected_status = 1;
     }

     struct create_args * create_args = (struct create_args *)malloc(sizeof(struct create_args));

     create_args->name = test_names[i];
     create_args->stinger_names_p = &names;
     create_args->expected_status = expected_status;
     create_args->expected_mapping = expected_out;

     register_test(th,
         "Create",
         &test_stinger_names_create,
         (void*)create_args);
   }

  run_tests(th,stderr);
  print_test_summary(th, stderr);
}



