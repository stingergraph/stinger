#include "histogram.h"
#include "xmalloc.h"

#include <stdio.h>

void
histogram_label_counts(struct stinger * S, uint64_t * labels, int64_t count, char * path, char * name, int64_t iteration) {
  uint64_t * histogram = xcalloc(sizeof(uint64_t), count+1);
  uint64_t * sizes = xcalloc(sizeof(uint64_t), count);

  if(!histogram || !sizes) {
    if(histogram) free(histogram);
    if(sizes) free(sizes);
    fprintf(stderr,"%s %d ALLOC FAIL \n", __func__, __LINE__); fflush(stdout);
    return;
  }

  for(uint64_t v = 0; v < count; v++) {
    if(stinger_vtype(S, v) != 0) {
      sizes[labels[v]]++;
    }
  }

  for(uint64_t v = 0; v < count; v++) {
    histogram[sizes[v]]++;
  }

  free(sizes);

  char filename[1024];
  sprintf(filename, "%s/%s.%" PRId64 ".csv", path, name, iteration);
  FILE * fp = fopen(filename, "w");
  for(uint64_t v = 1; v < count+1; v++) {
    if(histogram[v]) {
      fprintf(fp, "%" PRIu64 ", %" PRIu64 "\n", v, histogram[v]);
    }
  }
  fclose(fp);
  free(histogram);
}

void
histogram_int64(struct stinger * S, int64_t * scores, int64_t count, char * path, char * name, int64_t iteration) {
  int64_t max = 0;
  for(uint64_t v = 0; v < count; v++) {
    if(stinger_vtype(S, v) != 0) {
      if((scores[v]) > max) {
	max = (scores[v]);
      }
    }
  }

  uint64_t * histogram = xcalloc(sizeof(uint64_t), (max+2));

  if(histogram) {
    for(uint64_t v = 0; v < count; v++) {
      if(stinger_vtype(S, v) != 0) {
	histogram[scores[v]]++;
      }
    }

    char filename[1024];
    sprintf(filename, "%s/%s.%" PRId64 ".csv", path, name, iteration);
    FILE * fp = fopen(filename, "w");
    for(uint64_t v = 0; v < max+2; v++) {
      if(histogram[v]) {
	fprintf(fp, "%" PRIu64 ", %" PRIu64 "\n", v, histogram[v]);
      }
    }
    fclose(fp);
    free(histogram);
  } else {
    fprintf(stderr,"%s %d ALLOC FAIL \n", __func__, __LINE__); fflush(stdout);
  }
}

void
histogram_float(struct stinger * S, float * scores, int64_t count, char * path, char * name, int64_t iteration) {
  int64_t max = 0;
  for(uint64_t v = 0; v < count; v++) {
    if(stinger_vtype(S, v) != 0) {
      if((int64_t)(scores[v]) > max) {
	max = (int64_t) (scores[v]);
      }
    }
  }

  uint64_t * histogram = xcalloc(sizeof(uint64_t), (max+2));

  if(histogram) {
    for(uint64_t v = 0; v < count; v++) {
      if(stinger_vtype(S, v) != 0) {
	histogram[(int64_t)(scores[v])]++;
      }
    }

    char filename[1024];
    sprintf(filename, "%s/%s.%" PRId64 ".csv", path, name, iteration);
    FILE * fp = fopen(filename, "w");
    for(uint64_t v = 0; v < max+2; v++) {
      if(histogram[v]) {
	fprintf(fp, "%" PRIu64 ", %" PRIu64 "\n", v, histogram[v]);
      }
    }
    fclose(fp);
    free(histogram);
  } else {
    fprintf(stderr,"%s %d ALLOC FAIL \n", __func__, __LINE__); fflush(stdout);
  }
}

void
histogram_double(struct stinger * S, double * scores, int64_t count, char * path, char * name, int64_t iteration) {
  int64_t max = 0;
  for(uint64_t v = 0; v < count; v++) {
    if(stinger_vtype(S, v) != 0) {
      if((int64_t)(scores[v]) > max) {
	max = (int64_t) (scores[v]);
      }
    }
  }

  uint64_t * histogram = xcalloc(sizeof(uint64_t), (max+2));

  if(histogram) {
    for(uint64_t v = 0; v < count; v++) {
      if(stinger_vtype(S, v) != 0) {
	histogram[(int64_t)(scores[v])]++;
      }
    }

    char filename[1024];
    sprintf(filename, "%s/%s.%" PRId64 ".csv", path, name, iteration);
    FILE * fp = fopen(filename, "w");
    for(uint64_t v = 0; v < max+2; v++) {
      if(histogram[v]) {
	fprintf(fp, "%" PRIu64 ", %" PRIu64 "\n", v, histogram[v]);
      }
    }
    fclose(fp);
    free(histogram);
  } else {
    fprintf(stderr,"%s %d ALLOC FAIL \n", __func__, __LINE__); fflush(stdout);
  }
}
