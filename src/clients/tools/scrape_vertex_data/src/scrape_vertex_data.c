#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stinger_core/stinger_names.h"
#include "stinger_core/stinger.h"

int
main(int argc, char *argv[])
{
  if(argc < 3) {
    fprintf(stderr, "Usage: %s <vertex_file> <vertex_name> [<vertex_name> ... <vertex_name>]\n", argv[0]);
    fprintf(stderr, "   or: %s -i <vertex_file> <vertex_index> [<vertex_index> ... <vertex_index>]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "   Converts vertex name strings to integers and vice versa.\n");
    return -1;
  }

  int int_to_name = 0;

  if(0 == strcmp(argv[1], "-i")) {
    int_to_name = 1;
    argv++;
    argc--;
  }

  FILE * fp = fopen(argv[1], "r");

  int64_t max_len = 1024;
  int64_t nv = 0;

  fread(&max_len, sizeof(int64_t), 1, fp);
  fread(&nv, sizeof(int64_t), 1, fp);

  if(argc == 3) {

    if(int_to_name) {
      int64_t stop = atol(argv[2]);

      int64_t len = 0;
      char name[max_len+1];
      int64_t i = 0;

      for(; i <= stop && !feof(fp); i++) {
	fread(&len, sizeof(int64_t), 1, fp);
        if (len > max_len) { fprintf(stderr, "Name too long, file may be corrupt.\n"); return -1; }
        else { fread(name, sizeof(char), len, fp); }
      }
      if(i != stop) {
	printf("%.*s", (int) len, name);
      } else {
	fprintf(stderr, "Number out of range\n");
      }
    } else {
      int64_t len = 0;
      char name[max_len+1];
      int found = 0;

      for(int64_t i = 0; !feof(fp); i++) {
	fread(&len, sizeof(int64_t), 1, fp);
        if (len > max_len) { fprintf(stderr, "Name too long, file may be corrupt.\n"); return -1; }
	else { fread(name, sizeof(char), len, fp); }
	name[len] = '\0';
	if(0 == strcmp(name, argv[2])) {
	  printf("%ld ", (long)i);
	  found = 1;
	  break;
	}
      }
      if(!found) {
	fprintf(stderr, "Name not found\n");
      }
    }

  } else {
    stinger_names_t * names = stinger_names_new(nv * 2);
    stinger_names_load(names, fp);

    if(int_to_name) {
      for(int64_t i = 2; i < argc; i++) {
	printf("%s ", stinger_names_lookup_name(names, atol(argv[i])));
      }
    } else {
      for(int64_t i = 2; i < argc; i++) {
	printf("%ld ", (long)stinger_names_lookup_type(names, argv[i]));
      }
    }

    stinger_names_free(&names);
  }

  fclose(fp);
}
