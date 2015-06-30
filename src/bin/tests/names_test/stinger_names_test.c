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

struct reverse_lookup_args {
  char * expected_mapping;
  stinger_names_t ** stinger_names_p;
  int64_t type;
};

struct delete_args {
  int64_t type;
  stinger_names_t ** stinger_names_p;
};

struct delete_name_args {
  char * name;
  stinger_names_t ** stinger_names_p;
};

struct resize_args {
  stinger_names_t ** stinger_names_p;
  int64_t new_size;
};

struct print_args {
  stinger_names_t ** stinger_names_p;
};

void test_stinger_names_create(test_harness_t * th, void * arg) {
  struct create_args * args = (struct create_args *)arg;
  char * name = args->name;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  int64_t out = 0;
  fprintf(stderr,"Create %s\n", name);
  int64_t status = stinger_names_create_type(stinger_names, name, &out);
  TEST_ASSERT_EQ(status,args->expected_status);
  TEST_ASSERT_EQ(out,args->expected_mapping);
  if (FAILED_TEST) {
    fprintf(stderr,"status - Observed: %lld, Expected: %lld\n", status, args->expected_status);
    fprintf(stderr,"mapping - Observed: %lld, Expected: %lld\n", out, args->expected_mapping);
  }
}

void test_stinger_names_resize(test_harness_t * th, void * arg) {
  struct resize_args * args = (struct resize_args *)arg;
  int64_t new_size = args->new_size;
  fprintf(stderr,"Resizing to %ld\n", new_size);
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
  fprintf(stderr,"Lookup %s\n", name);
  out = stinger_names_lookup_type(stinger_names, name);
  TEST_ASSERT_EQ(out,args->expected_mapping);
  if (FAILED_TEST) {
    fprintf(stderr,"mapping - Observed: %lld, Expected: %lld\n", out, args->expected_mapping);
  }
}

void test_stinger_names_reverse_lookup(test_harness_t * th, void * arg) {
  struct reverse_lookup_args * args = (struct reverse_lookup_args *)arg;
  int64_t type = args->type;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  fprintf(stderr,"Lookup %ld\n", type);
  const char * out = stinger_names_lookup_name(stinger_names, type);
  
  if (args->expected_mapping != NULL) {
    TEST_ASSERT_NEQ(out,NULL);
    if (!FAILED_TEST) {
      TEST_ASSERT_STRCMP(out,args->expected_mapping);
    }
  } else {
    TEST_ASSERT_EQ(out,args->expected_mapping);
  }
  if (FAILED_TEST) {
    fprintf(stderr,"mapping - Observed: %s, Expected: %s\n", out, args->expected_mapping);
  }
}

void test_stinger_names_deletion(test_harness_t * th, void * arg) {
  struct delete_args * args = (struct delete_args *)arg;
  int64_t type = args->type;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  fprintf(stderr,"Delete %ld\n", type);
  int64_t rc = stinger_names_remove_type(stinger_names, type);
  TEST_ASSERT_EQ(rc,0);
  if (FAILED_TEST) {
    fprintf(stderr,"rc - Observed: %ld, Expected: %ld\n", rc, 0);
  }
}

void test_stinger_names_name_deletion(test_harness_t * th, void * arg) {
  struct delete_name_args * args = (struct delete_name_args *)arg;
  const char * name = args->name;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  fprintf(stderr,"Delete %s\n", name);
  int64_t rc = stinger_names_remove_name(stinger_names, name);
  TEST_ASSERT_EQ(rc,0);
  if (FAILED_TEST) {
    fprintf(stderr,"rc - Observed: %ld, Expected: %ld\n", rc, 0);
  }
}

void test_stinger_names_print(test_harness_t * th, void * arg) {
  struct print_args * args = (struct print_args *)arg;
  stinger_names_t * stinger_names = *args->stinger_names_p;
  stinger_names_print(stinger_names);
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

  struct print_args * print_args = (struct print_args *)malloc(sizeof(struct print_args));
  print_args->stinger_names_p = &names;

  register_test(th,
      "Print",
      &test_stinger_names_print,
      (void*)print_args);

  // Try reverse lookup
  struct reverse_lookup_args * reverse_lookup_args = (struct reverse_lookup_args *)malloc(sizeof(struct reverse_lookup_args));

  reverse_lookup_args->type = 2;
  reverse_lookup_args->stinger_names_p = &names;
  reverse_lookup_args->expected_mapping = "Gamma";

  register_test(th,
      "Reverse Lookup",
      &test_stinger_names_reverse_lookup,
      (void*)reverse_lookup_args);

  #ifdef STINGER_NAME_DELETION
  struct delete_args * delete_args = (struct delete_args *)malloc(sizeof(struct delete_args));

  delete_args->type = 2;
  delete_args->stinger_names_p = &names;

  register_test(th,
      "Delete Type 2",
      &test_stinger_names_deletion,
      (void*)delete_args);

  reverse_lookup_args = (struct reverse_lookup_args *)malloc(sizeof(struct reverse_lookup_args));

  reverse_lookup_args->type = 2;
  reverse_lookup_args->stinger_names_p = &names;
  reverse_lookup_args->expected_mapping = NULL;
  
  register_test(th,
      "Reverse Lookup",
      &test_stinger_names_reverse_lookup,
      (void*)reverse_lookup_args);
 

  struct delete_name_args * delete_name_args = (struct delete_name_args *)malloc(sizeof(struct delete_name_args));
  
  delete_name_args->name = "Beta";
  delete_name_args->stinger_names_p = &names;

  register_test(th,
      "Delete Type \"Beta\"",
      &test_stinger_names_name_deletion,
      (void*)delete_name_args);

  struct lookup_args * lookup_args = (struct lookup_args *)malloc(sizeof(struct lookup_args));

  lookup_args->name = "Beta";
  lookup_args->stinger_names_p = &names;
  lookup_args->expected_mapping = -1;

  register_test(th,
      "Lookup",
      &test_stinger_names_lookup,
      (void*)lookup_args);

  #endif

  run_tests(th,stderr);
  print_test_summary(th, stderr);
}



