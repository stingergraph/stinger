#ifndef  DEFS_HPP
#define  DEFS_HPP

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int64_t	  edgeType_t;
typedef uint64_t  vertexID_t;
typedef int64_t	  timeStamp_t;
typedef	uint64_t  ebIndex_t;
typedef uint64_t  degree_t;

#define stingerThrowIf(C,X) if(C) { fprintf(stderr, "%s @ %d: %s", __func__, __LINE__, (X)); \
  fflush(stderr); exit(-1); }

#endif  /*DEFS_HPP*/
