#ifndef  HISTOGRAM_H
#define  HISTOGRAM_H

#include "stinger.h"

#include <stdint.h>

void
histogram_label_counts(struct stinger * S, uint64_t * labels, int64_t count, char * path, char * name, int64_t iteration);

void
histogram_int64(struct stinger * S, int64_t * scores, int64_t count, char * path, char * name, int64_t iteration);

void
histogram_float(struct stinger * S, float * scores, int64_t count, char * path, char * name, int64_t iteration);

void
histogram_double(struct stinger * S, double * scores, int64_t count, char * path, char * name, int64_t iteration);

#endif  /*HISTOGRAM_H*/
