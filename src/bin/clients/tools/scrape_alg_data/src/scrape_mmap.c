#define _POSIX_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* 
 * alg state data format (binary):
 *  64-bit integer: name_length
 *  string: name
 *  64-bit integer: description_length
 *  string: description (see stinger_alg.h for explanation)
 *  64-bit integer: maximum number of vertices
 *  64-bit integer: number of vertices in file
 *  for each field in the description
 *    [ array of number of vertices elements of the given field type ]
 */
int main(int argc, char *argv[]) {
  int64_t page_size = sysconf(_SC_PAGE_SIZE);

  if(argc < 4) {
    fprintf(stderr, "Usage: %s <file> <field> <offset>\n", argv[0]);
    fprintf(stderr, "   or: %s -o <file> <field_number> <offset>\n", argv[0]);
    fprintf(stderr, "   or: %s -s <file> <field or field_number> <offset>\n", argv[0]);
    fprintf(stderr, "          + prints the type and raw offset into the file\n");
    fprintf(stderr, "          + also works with -o flag\n");
    fprintf(stderr, "   or: %s -d <file> <type> <skip>\n", argv[0]);
    fprintf(stderr, "          + uses the raw offset into the file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "       This program can be used to parse data out of algorithm files stored\n");
    fprintf(stderr, "       by the STINGER server.  It should be fairly quick when scraping a \n");
    fprintf(stderr, "       single value from a large number of results files.\n");
    return -1;
  }

  int offset_mode = 0;
  int print_raw_offset = 0;
  int raw_offset_mode = 0;

  int64_t skip = 0;
  int64_t nv = 0;
  char * types = NULL;
  char * desc = NULL;

  if(0 == strcmp(argv[1], "-o")) {
    offset_mode = 1;
    argv++;
  }
  if(0 == strcmp(argv[1], "-s")) {
    print_raw_offset = 1;
    argv++;
  }
  if(0 == strcmp(argv[1], "-d")) {
    raw_offset_mode = 1;
    argv++;

    types = argv[2];
    skip = atol(argv[3]);
  }

  if (!raw_offset_mode) {
    FILE * fp = fopen(argv[1], "r");

    /* skip name */
    int64_t size = 0;
    skip += sizeof(int64_t);
    fread(&size, sizeof(int64_t), 1, fp);
    char * tmp = malloc((size + 1) * sizeof(char));
    skip += size;
    fread(tmp, sizeof(char), size, fp);
    free(tmp);

    /* get description */
    skip += sizeof(int64_t);
    fread(&size, sizeof(int64_t), 1, fp);
    desc = calloc((size + 1), sizeof(char));
    skip += size;
    fread(desc, sizeof(char), size, fp);

    skip += sizeof(int64_t);
    fread(&nv, sizeof(int64_t), 1, fp); /* skip nv max */
    skip += sizeof(int64_t);
    fread(&nv, sizeof(int64_t), 1, fp);

    char * rest, * token, * ptr;

    ptr = desc;
    types = strtok_r(ptr, " ", &rest);
    ptr = rest;

    if(!types) {
      fprintf(stderr, "Error: the description string had no types!\n");
      return -1;
    }

    int64_t offset = 0;
    while(token = strtok_r(ptr, " ", &rest)) {
      if(0 == strcmp(argv[2], token) || (offset_mode && offset == atol(argv[2]))) {
	break;
      } else {
	switch(*types) {
	  case 'f': {
	    skip += nv * sizeof(float);
	  } break;
	  case 'd': {
	    skip += nv * sizeof(double);
	  } break;
	  case 'i': {
	    skip += nv * sizeof(int32_t);
	  } break;
	  case 'l': {
	    skip += nv * sizeof(int64_t);
	  } break;
	  case 'b': {
	    skip += nv * sizeof(uint8_t);
	  } break;

	  default: {
	    fprintf(stderr, "Error: description string contained unknown type: %c\n", *types);
	    return -1;
	  } break;
	}
	types++;
	offset++;
	ptr = rest;
      }
    }

    fclose(fp);
  }

  int64_t vtx = raw_offset_mode ? 0 : atol(argv[3]);

  if(print_raw_offset) {
    switch(*types) {
      case 'f': {
	skip += vtx * sizeof(float);
      } break;
      case 'd': {
	skip += vtx * sizeof(double);
      } break;
      case 'i': {
	skip += vtx * sizeof(int32_t);
      } break;
      case 'l': {
	skip += vtx * sizeof(int64_t);
      } break;
      case 'b': {
	skip += vtx * sizeof(uint8_t);
      } break;

      default: {
	fprintf(stderr, "Error: description string contained unknown type: %c\n", *types);
	return -1;
      } break;
    }
    printf("%c %ld", *types, (long)skip);
  } else {
    if(vtx <= nv) {
      int fd = open(argv[1], O_RDONLY);

      switch(*types) {
	case 'f': {
	  skip += vtx * sizeof(float);
	  uint8_t * mapping = (uint8_t *)mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, page_size * (skip / page_size));
	  printf("%f", *((float *)(mapping + (skip % page_size))));
	  munmap(mapping, page_size);
	} break;
	case 'd': {
	  skip += vtx * sizeof(double);
	  uint8_t * mapping = (uint8_t *)mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, page_size * (skip / page_size));
	  printf("%lf", *((double *)(mapping + (skip % page_size))));
	  munmap(mapping, page_size);
	} break;
	case 'i': {
	  skip += vtx * sizeof(int32_t);
	  uint8_t * mapping = (uint8_t *)mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, page_size * (skip / page_size));
	  printf("%ld", (long int)*((int32_t *)(mapping + (skip % page_size))));
	  munmap(mapping, page_size);
	} break;
	case 'l': {
	  skip += vtx * sizeof(int64_t);
	  uint8_t * mapping = (uint8_t *)mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, page_size * (skip / page_size));
	  printf("%ld", (long int)*((int64_t *)(mapping + (skip % page_size))));
	  munmap(mapping, page_size);
	} break;
	case 'b': {
	  skip += vtx * sizeof(uint8_t);
	  uint8_t * mapping = (uint8_t *)mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, page_size * (skip / page_size));
	  printf("%ld", (long int)*((uint8_t *)(mapping + (skip % page_size))));
	  munmap(mapping, page_size);
	} break;

	default: {
	  fprintf(stderr, "Error: description string contained unknown type: %c\n", *types);
	  return -1;
	} break;
      }

      close(fd);
    } else {
      printf("0");
    }
  }

  if(desc)
    free(desc);
}
